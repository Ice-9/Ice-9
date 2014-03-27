
# include   "standard.hpp"
# include   "memory9.hpp"

namespace Ice9 {
    namespace Memory {

        // PUBLIC
        void Allocator::Init() {
            alloc.checkNo = 0;
            alloc.cntRoots = 0;
            alloc.cntRootMarkers = 0;
            memset(alloc.npFrees, 0, sizeof(alloc.npFrees));

            alloc.nBase.nh_bszNode = 0;
            alloc.nBase.npNext = &alloc.nBase;
            alloc.nBase.nh__w = 0;

            alloc.npAllocp = &alloc.nBase;
            alloc.npChaser = Node::NIL;
            alloc.extents = NULL;

            alloc.qepList = NULL;
            alloc.qpFree = NULL;
            alloc.nQual = 0;

            alloc.bszStrWanted = 0;
            alloc.strBase = alloc.strFree = CharPtr(malloc(STR_EXPAND));
            // TODO: ERROR TRAP!!!
            alloc.strEnd = alloc.strBase + STR_EXPAND;

            alloc.qppBase = alloc.qppFree = alloc.qppEnd = NULL;
            alloc.szPost = 0;

        }   /* Allocator::Init */


        // this one is just for testing; forces immediate allocation of regions
        void Allocator::MakeSpaces() {
            QualifierExtent::AllocateQe(512, alloc.qepList, alloc.qpFree);
            Allocator::ExpandArena(10);     // why not...
        }


        // PUBLIC
        void Allocator::Destroy() {
            QualifierExtent::Ptr qep;
            Extent::Ptr ep;

            // return all extents of this allocator to the malloc pool
            while(ep = alloc.extents) {
                alloc.extents = ep->epNext;
                free(ep);
            }

            while(qep = alloc.qepList) {
                alloc.qepList = qep->qepNext;
                free(qep);
            }

        }   /* Allocator::Destroy */


        QualPtr Allocator::AllocQp(Word bsz, CharPtr str) {
            if(! alloc.qpFree)
                QualifierExtent::AllocateQe(512, alloc.qepList, alloc.qpFree);

            QualPtr result = alloc.qpFree;
            alloc.qpFree = alloc.qpFree->qpNext;
            ++alloc.nQual;

            result->nBytes = bsz;
            result->str = str;

            return result;
        }   /* Allocator::AllocQp */


        void Allocator::FreeQp(QualPtr qp) {
            --alloc.nQual;
            qp->qpNext = alloc.qpFree;
            alloc.qpFree = qp;
        }   /* Allocator::FreeQp */


        CharPtr Allocator::StrReserve(Word bsz) {
            CharPtr result;

            if(alloc.strFree + bsz > alloc.strEnd) {
                // Need to collect garbage.  To reduce thrashing, set a minimum requirement
                // of an extra 10 or 20% of the size of the region.
                //
                alloc.bszStrWanted = (alloc.strEnd - alloc.strBase) / 100 * 20;       // 20% of current size

                // bump this request to allocate with headroom
                // - otherwise a new collect will shortly follow!
                //
                Word bszNew = bsz / 100 * 120;

                // and bump the request if bsz swamps it
                if(alloc.bszStrWanted < bszNew)
                    alloc.bszStrWanted = bszNew;

                ReclaimInaccessibleObjects();

                if(alloc.strFree + bsz > alloc.strEnd)
                    ABORT("Botched string allocation\n");
            }

            result = alloc.strFree;
            alloc.strFree += bsz;
            return result;

        }   /* Allocator::StrReserve */


        // Pass a reference to the Qualifier record, so it's pointer can be adjusted.
        // Also, the HI bit of the size is set to show which Qualifiers are still live.
        // Remaining qualifiers can be put back into the free list.
        //
        void Allocator::PostQual(OP op) {
            QualPtr *qppNew;

            assert(op.isQual() || op.isSym());
            QualPtr qp = op.qpFrom();
            if(qp->marked)
                return;     // already marked

            if(qp->managed) {
                // The string is in the string space, and it's not marked.
                // Add it to the string qualifier list, but before adding it,
                // expand the string qualifier list if necessary.
                //
                if(alloc.qppFree >= alloc.qppEnd) {
                    // reallocate a new qualifier list that's twice as large
                    alloc.szPost *= 2;

                    qppNew = (QualPtr *) realloc(alloc.qppBase, alloc.szPost * sizeof(QualPtr));
                    if(! qppNew)
                        ABORT("Memory exhausted when allocating QualPost array\n");

                    alloc.qppFree = qppNew + (alloc.qppEnd - alloc.qppBase);
                    alloc.qppBase = qppNew;
                    alloc.qppEnd = alloc.qppBase + alloc.szPost;
                }

                *alloc.qppFree++ = qp;
            }
            else {
                // String is stored elsewhere (eg static space)
                // so we don't need to record it for compaction.
                // We do, however, have to mark it so it's not discarded. See below.
            }

            qp->marked = 1;

        }   /* Allocator::PostQual */


        // PUBLIC
        // register root objects such as Nil, NilClass, Object, ObjectClass, True, TrueClass, etc etc etc
        // TODO: the root array contains user-space pointers, not core. The marker needs to adjust when it uses it.
        // Can the marker mark only the sub-object referenced?
        //
        void Allocator::RegisterRoot(OP op) {
            assert(! op.isNil());
            assert(op.isRef());

            if(alloc.cntRoots >= MAX_ROOTS)
                ABORT("BUGGER: can't register any more roots\n");

            alloc.opRoots[alloc.cntRoots++] = op;
            op.npFrom()->nh_RC = RC_STUCK;
        }   /* Allocator::registerRoot */


        // PUBLIC
        void Allocator::DeregisterRoot(OP op) {
            assert(! op.isNil());
            assert(op.isRef());

            int i;
            for(i = 0; i < alloc.cntRoots; ++i)
                if(alloc.opRoots[i] == op)
                    break;

            if(i == alloc.cntRoots)
                ABORT("BUGGER: can't deregister root *%tX: not found\n", op);

            for(--alloc.cntRoots; i < alloc.cntRoots; ++i)
                alloc.opRoots[i] = alloc.opRoots[i + 1];

            DeleteNp(op);
        }   /* Allocator::DeregisterRoot */


        // PUBLIC
        // some things are not really registerable - eg way too many things on the stack
        // if things like stacks etc are not actual objects, use this to register a routine
        // which will traverse that thing and pass OID's that need to be treated as reachable.
        //
        // TODO: can this be used to register Qualifiers???
        //
        void Allocator::RegisterRootMarker(markerFtn hp) {
            if(alloc.cntRootMarkers >= MAX_ROOT_MARKERS)
                ABORT("BUGGER: can't register any more root markers\n");

            alloc.rootMarkers[alloc.cntRootMarkers++] = hp;
        }   /* Allocator::registerRootMarker */


        // PUBLIC
        void Allocator::DeregisterRootMarker(markerFtn hp) {
            int i;
            for(i = 0; i < alloc.cntRootMarkers; ++i)
                if(alloc.rootMarkers[i] == hp)
                    break;

            if(i == alloc.cntRootMarkers)
                ABORT("BUGGER: can't deregister root marker *%tX: not found\n", hp);

            for(--alloc.cntRootMarkers; i < alloc.cntRootMarkers; ++i)
                alloc.rootMarkers[i] = alloc.rootMarkers[i + 1];

        }   /* Allocator::deregisterRootMarker */


        // PUBLIC
        void Allocator::CheckArena(const char *const message) {
            int i;

            fprintf(stderr, ">>>>>>>>> Allocator::CheckArena No. %d: %s\n", ++alloc.checkNo, message);
            fprintf(stderr, "  EXTENTS:\n");
            i = 0;
            for(Extent::Ptr ep = alloc.extents; ep; ++i, ep = ep->epNext)
                fprintf(stderr, "    %8d: *%tX +: #%lX\n", i, ep, Word(ep->bsz));
            fprintf(stderr, "\n");

            NodePtr np = &alloc.nBase;
            fprintf(stderr, "  FREE LIST &nBase == *%tX\n", np);
            i = 0;
            while((np = np->npNext) != &alloc.nBase) {
                fprintf(stderr, "    %8d: *%tX +: #%lX x%d\n", i, np, np->nh_bszNode, np->nh_RC);
                if(++i > 100)
                    ABORT("Appear to be looping on the free list at *%tX\n", np);
            }
            fprintf(stderr, "\n");

            for(int q = 0; q < FREE_QUICKIES; ++q) {
                bool headed = false;
                for(i = 0, np = alloc.npFrees[q]; np; ++i, np = np->npNext) {
                    if(! headed) {
                        headed = true;
                        fprintf(stderr, "  QUICKIES: %d\n", q + 1);
                    }
                    fprintf(stderr, "    %8d: *%tX +: #%lx x%d\n", i, np, np->nh_bszNode, np->nh_RC);
                }
            }

            fprintf(stderr, "  HEAP WALK:\n");
            i = 0;
            for(Extent::Ptr ep = alloc.extents; ep; ++i, ep = ep->epNext)
                for(NodePtr np = NodePtr(ep->wArray); np < npAddBsz(NodePtr(ep->wArray), ep->bsz); np = npAddBsz(np, np->nh_bszNode))
                    fprintf(stderr, "    %8d: *%tX +: #%lX x%d\n", i, np, np->nh_bszNode, np->nh_RC);

            fprintf(stderr, "\n");

            fprintf(stderr, "  QUALIFIERS:\n");
            for(QualifierExtent::Ptr qep = alloc.qepList; qep; qep = qep->qepNext) {
                fprintf(stderr, "    qep: *%tX +: %d\n", qep, qep->nq);
                for(i = 0; i < qep->nq; ++i) {
                    QualPtr qp = &qep->qArray[i];
                    // The following finds legit strings in the string area,
                    // finding and printing quals that point to C strings.
                    // This is tricky 'cos we have to prove it's not pointing
                    // to the qualifier extents - ie that would make it
                    // a member of the free list.
                    //
                    if(! qp->str)       // ignore null pointer
                        continue;

                    // is it a member of the string region?
                    if(! qp->managed) {
                        // if it's not, final check is to prove it's not in the free list
                        QualifierExtent::Ptr qep1;
                        for(qep1 = alloc.qepList; qep1; qep1 = qep1->qepNext)
                            if(CharPtr(&qep1->qArray[0]) <= qp->str && qp->str < CharPtr(&qep1->qArray[qep1->nq]))
                                break;

                        if(qep1)        // found in free list so skip printing
                            continue;
                    }

                    fprintf(stderr, "    *%tX is: *%tX +: #%lX %.*s\n", qp, qp->str, qp->nBytes, qp->nBytes, qp->str);
                }
            }
            fprintf(stderr, "\n");

            fprintf(stderr, "<<<<<<<<< Allocator::CheckArena No. %d\n", alloc.checkNo);
            fprintf(stderr, "\n");
        }   /* Allocator::CheckArena */


        void Allocator::Marker(OP op) {
            // NOTE: only objects get into the root list,
            // but the root-marker callbacks need to ONLY pass objects.
            // probably a good idea to check the fact here. spit the dummy if it gets an int or qual??
            // TODO: oh crap - a qual hidden away elsewhere will probably not survive a string collection,
            // so the string collectors need to make some use of the root markers?
            //
            if(! op.isNil() && op.isRef()) {
                if(op.npFrom()->nh_RC == 0) {
                    op.npFrom()->nh_RC = 1;
                    MarkAccessibleObjects(op);
                }
            }
            else if(op.isQual() || op.isSym())
                PostQual(op);
        }   /* Allocator::Marker */


        // PUBLIC
        void Allocator::ReclaimInaccessibleObjects() {
            int i;

            alloc.szPost = 1024;
            alloc.qppFree = alloc.qppEnd = alloc.qppBase;

            ZeroRefCounts();

            for(i = 0; i < alloc.cntRoots; ++i)
                Marker(alloc.opRoots[i]);

            for(i = 0; i < alloc.cntRootMarkers; ++i)
                (*alloc.rootMarkers[i])(&Allocator::Marker);

            RectifyCountsAndDeallocateGarbage();

            // compact the string space
            StringsReclaim();

        }   /* Allocator::ReclaimInaccessibleObjects */


        static int qppCmp(const void *l, const void *r) {
            Int dif = QualPtr(l)->str - QualPtr(r)->str;
            if(dif < 0) return -1;
            if(dif > 0) return  1;
            return 0;
        }   /* qppCmp */


        void Allocator::StringsReclaim() {
            QualifierExtent::Ptr qep;
            Int i;
            QualPtr qp;

            alloc.nQual = 0;
            alloc.qpFree = NULL;

            if(alloc.qppBase >= alloc.qppFree) {
                // no strings so just splat everything
                alloc.strFree = alloc.strBase;

                // trash all the Qualifier extents too
                while(qep = alloc.qepList) {
                    alloc.qepList = qep->qepNext;
                    free(qep);
                }
            }
            else {
                // compact the stringy area
                // Sort the pointers on qppBase in ascending order of string locations.
                qsort((char *) alloc.qppBase, alloc.qppFree - alloc.qppBase, sizeof(*alloc.qppBase), qppCmp);

                CharPtr strDest = alloc.strBase;
                CharPtr strSrc = alloc.qppBase[0]->str;
                CharPtr strCend = strSrc;

                // Loop through qualifiers for accessible strings.
                for(QualPtr *qpp = alloc.qppBase; qpp < alloc.qppFree; ++qpp) {
                    qp = *qpp;
                    if(qp->str > strCend) {
                        // qpp points to a qualifier for a string in the next clump.
                        // The last clump is moved, and strSrc and strCend are set for the next clump.
                        //
                        while(strSrc < strCend)
                            *strDest++ = *strSrc++;
                        strSrc = strCend = qp->str;
                    }

                    if(qp->str + qp->nBytes > strCend)
                        // qpp is a qualifier for a string in this clump; extend the clump.
                        strCend = qp->str + qp->nBytes;

                    // Relocate the string qualifier.
                    qp->str += (strDest - strSrc);
                }

                // Move the last clump.
                while(strSrc < strCend)
                    *strDest++ = *strSrc++;
                alloc.strFree = strDest;

                // Travel the Qualifier extents and rebuild the free list. Also, clear the marks.
                for(qep = alloc.qepList; qep; qep = qep->qepNext) {
                    // TODO: could count usage within the block and splat the whole thing if it's totally unused.

                    for(i = qep->nq, qp = qep->qArray + i - 1; i > 0; --i, --qp) {
                        if(qp->marked) {
                            qp->marked = 0;
                            ++alloc.nQual;
                        }
                        else {
                            // chuck it on the free list
                            qp->qpNext = alloc.qpFree;
                            alloc.qpFree = qp;
                            qp->str = NULL;
                        }
                    }
                }
            }

            if(alloc.strEnd - alloc.strFree < alloc.bszStrWanted) {
                // need to expand the string area - add the desired extra
                Word bsz = alloc.strEnd - alloc.strBase + alloc.bszStrWanted;

                // expand the string area
                CharPtr strNew = CharPtr(realloc(alloc.strBase, bsz));

                if(! strNew)
                    ABORT("Memory exhausted when expanding string region\n");

                // calculate ofsetting amount
                Int ofs = strNew - alloc.strBase;

                // Loop through qualifiers and adjust addresses
                for(QualPtr *qpp = alloc.qppBase; qpp < alloc.qppFree; ++qpp)
                    if((*qpp)->managed)
                        (*qpp)->str += ofs;

                alloc.strFree = strNew + (alloc.strFree - alloc.strBase);
                alloc.strBase = strNew;
                alloc.strEnd  = alloc.strBase + bsz;
            }

        }   /* Allocator::StringsReclaim */


        void Allocator::ZeroRefCounts() {
            for(Extent::Ptr ep = alloc.extents; ep; ep = ep->epNext)
                for(NodePtr np = NodePtr(ep->wArray); np < npAddBsz(NodePtr(ep->wArray), ep->bsz); np = npAddBsz(np, np->nh_bszNode))
                    np->nh_RC = 0;
        }   /* Allocator::zeroRefCounts */


        void Allocator::MarkAccessibleObjects(OP op) {
            // compute prior, current, offset, and size to begin processing objectPointer
            OP opPrior = NIL, opChild = NIL, opCurrent = op;
            Word offset, size, dimSkip;
            dimSkip = opCurrent.StartWord();
            offset = opCurrent.LimitPtr();
            size = opCurrent.LimitWord();

            for(;;) {       // for all pointers in all objects traversed
                if(offset > dimSkip) {
                    --offset;
                    opChild = opCurrent.npFrom()->opChilds[dimSkip + offset];      // one of the pointers

                    if(opChild.isNil() || opChild.isInt())
                        continue;

                    if(opChild.isQual() || op.isSym()) {
                        PostQual(opChild);
                        continue;
                    }

                    if(opChild.isRef() && opChild.npFrom()->nh_RC == 0) {
                        // it's a non-immediate object and it should be processed
                        opChild.npFrom()->nh_RC = 1;

                        // if the child object is binary, don't need to dive into it.
                        if(opChild.isBin())
                            continue;

                        // reverse the pointer chain
                        opCurrent.npFrom()->opChilds[dimSkip + offset] = opPrior;

                        // save the offset either in the count field or in the extra word
                        if(opCurrent.npFrom()->nh_oversize)
                            opCurrent.npFrom()->wChilds[size] = offset + 1;
                        else
                            opCurrent.npFrom()->nh_RC = offset + 1; // TODO: check the oversize is set to allow + 1 which
                                                                    // is needed to make the RC non-zero under the
                                                                    // constraint that our offsets CAN be zero for the class

                        // compute opPrior, opCurrent, offset, and size to begin processing opChild
                        opPrior = opCurrent;
                        opCurrent = opChild;
                        dimSkip = opCurrent.StartWord();
                        offset = opCurrent.LimitPtr();
                        size = opCurrent.LimitWord();
                    }
                }
                else {
                    // all pointers have been followed; now perform the action
                    // did we get here from another object?
                    if(opPrior == NIL)       // this was the root object, so we are done
                        break;

                    // restore opChild, opCurrent, and size to resume processing prior
                    opChild = opCurrent;
                    opCurrent = opPrior;

                    dimSkip = opCurrent.StartWord();
                    size = opCurrent.LimitWord();

                    // restore offset either from the count field or from the extra word
                    if(opCurrent.npFrom()->nh_oversize)
                        offset = opCurrent.npFrom()->wChilds[size] - 1;
                    else
                        offset = opCurrent.npFrom()->nh_RC - 1;

                    // restore opPrior from the reversed pointer chain
                    // and restore (unreverse) the pointer chain
                    opPrior = opCurrent.npFrom()->opChilds[dimSkip + offset];
                    opCurrent.npFrom()->opChilds[dimSkip + offset] = opChild;
                    opCurrent.npFrom()->nh_RC = 1;
                }
            }

        }   /* Allocator::MarkAccessibleObjects */


        void Allocator::PlainFree(NodePtr np) {
            Word nChunks = np->nh_bszNode / ALIGN_SIZE_BYTES - 1;   // -1 'cos there'll never be zero
            if(nChunks < FREE_QUICKIES) {
                np->npNext = alloc.npFrees[nChunks];
                alloc.npFrees[nChunks] = np;
            }
            else {
                np->npNext = alloc.npAllocp->npNext;
                alloc.npAllocp->npNext = np;
            }
        }   /* Allocator::PlainFree */


        void Allocator::RectifyCountsAndDeallocateGarbage() {
            // zero all the free lists
            memset(alloc.npFrees, 0, sizeof(alloc.npFrees));
            alloc.npAllocp = &alloc.nBase;
            alloc.nBase.npNext = &alloc.nBase;

            // rectify counts, and deallocate garbage
            NodePtr npPrev = Node::NIL;
            Word step;
            for(Extent::Ptr ep = alloc.extents; ep; ep = ep->epNext) {
                for(NodePtr np = NodePtr(ep->wArray); np < npAddBsz(NodePtr(ep->wArray), ep->bsz); np = npAddBsz(np, step)) {
                    step = np->nh_bszNode;

                    if(np->nh_RC == 0) {       // if this object has no count,
                        // scrap it
                        // TODO: what about zeroing binary objects?? when does that happen?
                        if(np->nh_isptrs)
                            memset(np->opChilds, 0, np->nh_bszNode - NHDR_SIZE);

                        np->nh__w = 0;
                        np->npNext = Node::NIL;

                        if(! npPrev)        // first block found?
                            npPrev = np;
                        else if(npAddBsz(npPrev, npPrev->nh_bszNode) != np) {
                            // shove npPrev on free list
                            PlainFree(npPrev);
                            npPrev = np;
                        }
                        else {
                            // glue together with prev
                            npPrev->nh_bszNode += np->nh_bszNode;
                            np->nh_bszNode = 0;
                        }

                        continue;
                    }

                    // let it live
                    np->CountDown();        // compensate for the mark

                    Word last = np->LimitWord();
                    for(Word offset = np->StartWord(); offset < last; ++offset) {
                        OP op1 = np->opChilds[offset];
                        if(! op1.isNil() && op1.isRef())
                            op1.CountUp();
                    }
                }
            }

            if(npPrev)
                PlainFree(npPrev);

            // ensure the roots don't disappear
            for(Word i = 0; i < alloc.cntRoots; ++i)
                alloc.opRoots[i].npFrom()->nh_RC = RC_STUCK;

        }   /* Allocator::RectifyCountsAndDeallocateGarbage */


        // ask for n-words of space.
        // may be called when the free list is not empty, so it must insert
        // into the free list, not just assign a lump to the pointer
        //
        void Allocator::ExpandArena(Word bsz) {
            // ensure it's a multiple of HEAP_EXPAND
            bsz = HEAP_EXPAND * ((bsz + HEAP_EXPAND - 1) / HEAP_EXPAND);
            if(bsz < HEAP_EXPAND)
                bsz = HEAP_EXPAND;

            // TODO: check bsz doesn't exceed sensibility bounds

            // allocate and record new Extent
            Extent::Ptr epNew = Extent::epAlloc(bsz + sizeof(Extent));
            epNew->bsz = bsz;
            epNew->epNext = alloc.extents;
            alloc.extents = epNew;

        // SOMETHING DODGY HERE??
            // int taken = ALIGN_SIZE_BYTES > sizeof(Extent) ? ALIGN_SIZE_BYTES : sizeof(Extent);
            // taken = ALIGN_SIZE_BYTES * ((taken + ALIGN_SIZE_BYTES - 1) / ALIGN_SIZE_BYTES);

            NodePtr npNew = NodePtr(epNew->wArray);

            npNew->nh_bszNode = bsz;       // record the freshly allocated size of the lump
            npNew->nh__w = 0;
            // need to insert in proper place since the free list is in address-order for coalescing purposes
            FreeNp(npNew);
        }   /* Allocator::ExpandArena */


        void Allocator::DeferredDown(NodePtr np) {
            // Skip the first one 'cos it's already been commandeered as the free list pointer.
            // This assumes that all the dimensions have been splatted, INCLUDING zeroing the nh_dim field.
            Word limit = np->wszData();
            for(Word i = 1; i < limit; ++i) {
                OP opDel = np->opChilds[i];
                if(! opDel.isNil() && opDel.isRef())
                    DeleteNp(opDel);
            }

            np->nh__w = 0;
            memset(np->opChilds, 0, np->nh_bszNode - NHDR_SIZE);
        }   /* Allocator::DeferredDown */


        void Allocator::FreeListInsert(NodePtr np, NodePtr npPrev) {
            // TODO: joining blocks: if nh_isptrs doesn't match, coordinate the lumps.
            // Both would have to become nh_isptrs so the current nh_isptrs needs cleaning.
            // Bit of a bummer: time-waster, but at least the nh_isptrs block will be all Node::NIL
            // and fairly low-cost to scan in deferred down.
            // ALSO TODO: if the chunk is small enough to be on a quickie-list, put it there if it doesn't get merged?
            // then again, all the quickie-entries will interfere with coalescence. Maybe the balance is good.

            if(npAddBsz(np, np->nh_bszNode) != npPrev->npNext)     // join to upper neighbour?
                np->npNext = npPrev->npNext;        // can't join
            else {
                NodePtr npUpper = npPrev->npNext;

                // if either block is pointy, need to make the combo pointy so it
                // can safely participate in a deferred countDown
                //
                if(np->nh_isptrs != npUpper->nh_isptrs)
                    np->nh_isptrs = npUpper->nh_isptrs = 1;

                np->nh_bszNode += npUpper->nh_bszNode;
                np->npNext = npUpper->npNext;

                // special words of upper neighbour need NULLing now 'cos they
                // may be treated as child members during deferred countDown.
                // Need to do this EVEN IF it's currently not nh_isptrs, because
                // it might be glued to a pointy block later, and these words
                // will be non-null and buried in the middle somewhere.
                //
                npUpper->nh_bszNode = 0;
                npUpper->npNext = 0;
                npUpper->nh__w = 0;

                // if the upper neighbour is referred to by npChaser, we need to reassign it
                // to protect the integrity of the npAllocate routine.
                if(alloc.npChaser == npUpper)
                    alloc.npChaser = np;
                npPrev->npNext = np;
            }

            if(npAddBsz(npPrev, npPrev->nh_bszNode) != np)         // join to lower neighbour?
                npPrev->npNext = np;                // can't join
            else {
                // if either block is pointy, need to make the combo pointy so it
                // can safely participate in a deferred countDown
                //
                if(npPrev->nh_isptrs != np->nh_isptrs)
                    npPrev->nh_isptrs = np->nh_isptrs = 1;

                npPrev->nh_bszNode += np->nh_bszNode;
                npPrev->npNext = np->npNext;

                // special words of this block need NULLing now 'cos they
                // may be treated as child members during deferred countDown.
                // Need to do this EVEN IF it's currently not a nh_isptrs, because
                // it might be glued to a pointy block later, and these words
                // will be non-null and buried in the middle somewhere.
                //
                np->nh_bszNode = 0;
                np->npNext = 0;
                np->nh__w = 0;
            }
        }   /* Allocator::FreeListInsert */


        void Allocator::FreeNp(NodePtr npOld) {
            // walk the freelist and attempt to coalesce
            // may be able to join with entry before AND entry after

            // Need to zero the members 'cos deferred countdown in npAllocate HAS to assume all entries are pointers.
            // Even if full of pointers, the dimensions words need to be zeroed.
            //
            if(! npOld->nh_isptrs)
                memset(npOld->opChilds, 0, npOld->nh_bszNode - NHDR_SIZE);
            else {
                if(npOld->nh_dims)
                    memset(npOld->opChilds, 0, npOld->nh_dims * 2 * WORD_SIZE);
                if(npOld->nh_oversize)
                    npOld->wChilds[npOld->LimitWord()] = 0;
            }
            npOld->nh_dims = 0;

            int nChunks = npOld->nh_bszNode / ALIGN_SIZE_BYTES - 1;   // -1 'cos there'll never be zero
            if(nChunks < FREE_QUICKIES) {
                npOld->npNext = alloc.npFrees[nChunks];
                alloc.npFrees[nChunks] = npOld;
            }
            else {
                NodePtr npPrev;
                // find it's place in the free list, which is kept in address order.
                for(npPrev = alloc.npAllocp; npOld <= npPrev || npOld >= npPrev->npNext; npPrev = npPrev->npNext)
                    if(npPrev >= npPrev->npNext && (npOld > npPrev || npOld < npPrev->npNext))
                        break;      // at one end or other

                // now attach into place
                FreeListInsert(npOld, npPrev);
                alloc.npAllocp = npPrev;
            }

        }   /* Allocator::FreeNp */


        NodePtr Allocator::npAllocate(Word bsz) {
            // find and split a block
            // in 'ere, we work in total space needs, not end-user's space needs
            // bsz is expected to be units of bytes required

            // now make sure the allocation is in multiples of the alignment size
            Word extra = (bsz + ALIGN_SIZE_BYTES - 1) / ALIGN_SIZE_BYTES * ALIGN_SIZE_BYTES - bsz;
            bsz += extra;

            if(bsz < NODE_SIZE) {
                // need at least one word for the npNext pointer when on the free list
                extra += NODE_SIZE - bsz;
                bsz = NODE_SIZE;
            }

            // actually, if the node size is more than one ALIGN_SIZE_BYTE, there's never be 1 (or even 2...)
            int nChunks = bsz / ALIGN_SIZE_BYTES - 1;   // -1 'cos there'll never be zero
            NodePtr np;
            if(nChunks < FREE_QUICKIES && (np = alloc.npFrees[nChunks]))
                alloc.npFrees[nChunks] = np->npNext;
            else {
                // npChaser points to previous block in search so we can fix up links
                for(alloc.npChaser = alloc.npAllocp, np = alloc.npAllocp->npNext; ; np = (alloc.npChaser = np)->npNext) {
                    if(np->nh_bszNode == bsz               // exact match for requested size?
                    || np->nh_bszNode >= bsz + NODE_SIZE   // big enough to split?
                    ) {
                        // detach so the following deletes won't glue something to our found block
                        alloc.npChaser->npNext = np->npNext;    // step the free list around the given object

                        if(np->nh_isptrs)
                            DeferredDown(np);

                        if(np->nh_bszNode > bsz) {
                            // big enough to split
                            // TODO: split off the beginning to reduce activity at the far end of a new large chunk
                            //
                            np->nh_bszNode -= bsz;                         // shrink the found block
                            NodePtr np1 = npAddBsz(np, np->nh_bszNode);    // now point to the tail being allocated
                            np1->nh_bszNode = bsz;                         // and prepare it

                            nChunks = np->nh_bszNode / ALIGN_SIZE_BYTES - 1;   // -1 'cos there'll never be zero
                            if(nChunks < FREE_QUICKIES) {
                                np->npNext = alloc.npFrees[nChunks];
                                alloc.npFrees[nChunks] = np;
                            }
                            else
                                FreeListInsert(np, alloc.npChaser);               // reattach remaining chunk

                            np = np1;               // finally focus on the allocation
                        }

                        alloc.npAllocp = alloc.npChaser;        // start next allocation from this position - share the joy
                        break;
                    }

                    if(np == alloc.npAllocp)
                        ExpandArena(bsz);
                }
            }

            np->nh__w = 0;
            np->nh_extra = extra;

            // TODO: prove this memset is redundant
            memset(np->opChilds, 0, np->nh_bszNode - NHDR_SIZE);

            return np;

        }   /* Allocator::npAllocate */


        /*
         * NOTE: following chatter is not quite correct or up to date
         * ARGUMENTS:
         *      Word szEa = 0       size of each element. zero means allocating references (native size)
         *      int dims = 0, ...   number of dimensions followed by each lwb,upb
         *
         * If dims == 0, implies allocating a scalar object:
         * if szEa == 0 (ie pointer-ful object) then must be followed by size of the scalar object
         * if dims == 0 && szEa != 0 then no args follow on the call
         */

        /*
           scalar binary:           class, align, szEa, dims=0
           vector binary:           class, align, szEa, dims=n, ...

           scalar pointers:         class, 0,     0,    dims=0, size
           vector pointers:         class, 0,     0,    dims=n, ...
        */


        OP Allocator::opNewStr(Word bsz, const char *cp, bool managed) {
            // non-NULL cp is a pre-existing string to be pointed at
            // NULL cp implies a request to actually allocate
            //
            CharPtr str = CharPtr(cp);
            if(! str)
                str = StrReserve(bsz);

            QualPtr qp = AllocQp(bsz, str);
            qp->managed = managed;
            return opFromQual(qp);

        }   /* Allocator::opNewStr */


        // PUBLIC
        OP Allocator::opNewObj(OP opClass, Word align, Word szEa, Word sized, std::va_list ap) {
            Word sz, mofs, bsz, dims;
            bool oversize, addClass;

            Word dimensions[63][2];

            // align == 0 szEa == ? sized => nItems ap == NULL : allocating scalar MI object (NOTE: pass szEa as 0)
            // align == 0 szEa == ? sized => dims   ap != NULL : allocating array of OPs     (NOTE: pass szEa as 0)
            // align != 0 szEa == X sized => nItems ap == NULL : allocating scalar binary, size * dims (actually a kind of vector - shhhh)
            // align != 0 szEa == X sized => dims   ap != NULL : allocating array of binary
            //
            // tote up the number of items
            if(ap == NULL) {        // scalar?
                sz = sized;         // number of items
                dims = 0;
            }
            else {
                sz = 1;
                dims = sized;

                for(int i = 0; i < dims; ++i) {
                    Word lwb = va_arg(ap, Word);
                    Word upb = va_arg(ap, Word);
                    Word dim = upb - lwb + 1;
                    if(dim < 0)             // TODO: could it make sense to have backwards indexes???
                        dim = 0;            // TODO: throw an error probably, the runtime should not let this happen
                    sz *= dim;

                    dimensions[i][0] = lwb;
                    dimensions[i][1] = upb;
                }
            }

            // build up the size
            addClass = align != 0 || dims != 0;    // need to add a word for the classPointer?
            bsz = dims * 2 + addClass;

            if(align == 0)  // pointered
                bsz += sz;
            // if pointers AND too many of them to store the pointer count in nh_RC, need to allocate a slot for GC
            oversize = bsz > HUGE_SIZE;

            bsz *= WORD_SIZE;
            bsz += NHDR_SIZE;

            // NOW: bsz = total number of bytes before any binary data (which will be nothing if pointered)
            if(align == 0)
                mofs = 0;
            else {
                mofs = (bsz + align - 1) / align * align - bsz;
                bsz += sz * szEa;
            }

            if(addClass) mofs += WORD_SIZE;
            if(oversize) bsz += WORD_SIZE;

            NodePtr np = npAllocate(bsz);

            // store the final attributes
            np->nh_dims = dims;
            np->nh_oversize = oversize;
            np->nh_mofs = mofs;
            if(align == 0)
                np->nh_isptrs = 1;
            else
                np->nh_msize = szEa - 1;

            // store the dimensions into place
            for(int i = 0; i < dims; ++i) {
                np->wChilds[i * 2 + 0] = dimensions[i][0];
                np->wChilds[i * 2 + 1] = dimensions[i][1];
            }

            // if no class has been passed, becomes it's own class
            // ONLY for bootstrapping the first class...
            if(! VoidPtr(opClass))
                opClass = opFromRef(np);
            np->opChilds[dims * 2] = opClass;

            return opFromRef(np);
        }   /* Allocator::opNewObj */


        void Allocator::DeleteNp(OP op) {
            op.CountDown();
            if(op.npFrom()->nh_RC == 0)
                FreeNp(op.npFrom());
        }   /* Allocator::DeleteNp */


        __thread Ice9::Memory::Allocator Allocator::alloc;

    };  /* namespace Ice9::Memory */
};  /* namespace Ice9 */

