
# include  "standard.hpp"
# include  "memory9.hpp"

namespace Ice9 {
    namespace Memory {
        const OP NIL = opRaw(0);
        OP OP::ClassClass;
        OP OP::NilClass;
        OP OP::ObjectClass;
        OP OP::IntClass;
        OP OP::QualClass;
        OP OP::SymbolClass;
        OP OP::BooleanClass;
        OP OP::SymSetClass;
        OP OP::SymHashClass;
        OP OP::SymSetLinkClass;
        OP OP::SymHashLinkClass;

        OP OP::GlobalsHash;
        OP OP::SymbolsSet;


        void OP::registerClassClass(OP op)       { (ClassClass = op).registerRoot();       }
        void OP::registerNilClass(OP op)         { (NilClass = op).registerRoot();         }
        void OP::registerObjectClass(OP op)      { (ObjectClass = op).registerRoot();      }
        void OP::registerIntClass(OP op)         { (IntClass = op).registerRoot();         }
        void OP::registerQualClass(OP op)        { (QualClass = op).registerRoot();        }
        void OP::registerSymbolClass(OP op)      { (SymbolClass = op).registerRoot();      }
        void OP::registerBooleanClass(OP op)     { (BooleanClass = op).registerRoot();     }
        void OP::registerSymSetClass(OP op)      { (SymSetClass = op).registerRoot();      }
        void OP::registerSymHashClass(OP op)     { (SymHashClass = op).registerRoot();     }
        void OP::registerSymSetLinkClass(OP op)  { (SymSetLinkClass = op).registerRoot();  }
        void OP::registerSymHashLinkClass(OP op) { (SymHashLinkClass = op).registerRoot(); }


        void OP::StandardClasses() {
            OP op;

                  registerClassClass(op = opObjectNew(NIL, 0, 0, 2));        op.opSetAt(1, opFromInt(-2));
                    registerNilClass(op = opObjectNew(ClassClass, 0, 0, 2)); op.opSetAt(1, opFromInt(-2));
                 registerObjectClass(op = opObjectNew(ClassClass, 0, 0, 2)); op.opSetAt(1, opFromInt(-2));
                    registerIntClass(op = opObjectNew(ClassClass, 0, 0, 2)); op.opSetAt(1, opFromInt(-2));
                   registerQualClass(op = opObjectNew(ClassClass, 0, 0, 2)); op.opSetAt(1, opFromInt(-2));
                 registerSymbolClass(op = opObjectNew(ClassClass, 0, 0, 2)); op.opSetAt(1, opFromInt(-2));
                registerBooleanClass(op = opObjectNew(ClassClass, 0, 0, 2)); op.opSetAt(1, opFromInt(-2));
                 registerSymSetClass(op = opObjectNew(ClassClass, 0, 0, 2)); op.opSetAt(1, opFromInt(-2));
                registerSymHashClass(op = opObjectNew(ClassClass, 0, 0, 2)); op.opSetAt(1, opFromInt(-2));
             registerSymSetLinkClass(op = opObjectNew(ClassClass, 0, 0, 2)); op.opSetAt(1, opFromInt(-2));
            registerSymHashLinkClass(op = opObjectNew(ClassClass, 0, 0, 2)); op.opSetAt(1, opFromInt(-2));

        }   /* OP::StandardClasses */


        void OP::StandardObjects() {
            (SymbolsSet = NewSymSet()).registerRoot();
            (GlobalsHash = NewSymHash()).registerRoot();
        }   /* OP::StandardObjects */


        OP OP::NewSymSet() {
            OP op = opObjectNew(SymSetClass, 0, 0, SYM_SET::SIZE);
            op.opSetAt(SYM_SET::nSize, opFromInt(255));         // size
            op.opSetAt(SYM_SET::nEntries, opFromInt(0));        // nEntries
            op.opSetAt(SYM_SET::opTable, opObjectNew(ObjectClass, 0, 0, 256));     // TODO: a real class
            return op;
        }   /* OP::NewSymSet */


        OP OP::NewSymSetLink(OP opLink, OP opKey) {
            OP op = opObjectNew(SymSetLinkClass, 0, 0, SYM_SET_LINK::SIZE);
            op.opSetAt(SYM_SET_LINK::opLink, opLink);
            op.opSetAt(SYM_SET_LINK::opKey, opKey);
            return op;
        }   /* OP::NewSymSetLink */


        OP OP::NewSymHash() {
            OP op = opObjectNew(SymHashClass, 0, 0, SYM_HASH::SIZE);
            op.opSetAt(SYM_HASH::nSize, opFromInt(255));        // size
            op.opSetAt(SYM_HASH::nEntries, opFromInt(0));       // nEntries
            op.opSetAt(SYM_HASH::opTable, opObjectNew(ObjectClass, 0, 0, 256));     // TODO: a real class
            return op;
        }   /* OP::NewSymHash */


        OP OP::NewSymHashLink(OP opLink, OP opKey, OP opVal) {
            OP op = opObjectNew(SymHashLinkClass, 0, 0, SYM_HASH_LINK::SIZE);
            op.opSetAt(SYM_HASH_LINK::opLink, opLink);
            op.opSetAt(SYM_HASH_LINK::opKey, opKey);
            op.opSetAt(SYM_HASH_LINK::opVal, opVal);
            return op;
        }   /* OP::NewSymHashLink */


        OP OP::_HuntSymbol(Word *hash, const char *cp, Word len) {
            assert(this->isObj());
            assert(this->classOf() == SymSetClass);

            *hash = strHash(cp, len) % opGetAt(SYM_SET::nSize).iFrom() + 1;
            fprintf(stderr, "_HuntSymbol %d <%.*s> => %X\n", len, len, cp, *hash);

            OP opLink = opGetAt(SYM_SET::opTable).opGetAt(*hash);
            while(! opLink.isNil()) {
                OP opKey = opLink.opGetAt(SYM_SET_LINK::opKey);
                if(opKey.strLen() == len && 0 == strncmp(opKey.strLoc(), cp, len))
                    return opKey;

                // move on
                opLink = opLink.opGetAt(SYM_SET_LINK::opLink);
            }

            return NIL;
        }   /* OP::_HuntSymbol */


        OP OP::_StoreSymbol(Word hash, OP opSym) {
            // convert the OP to have Symbol tag bits
            opSym.i = (opSym.i & ~ OP_QUAL) | OP_SYM;
            OP opLink = NewSymSetLink(opGetAt(SYM_SET::opTable).opGetAt(hash), opSym);
            opGetAt(SYM_SET::opTable).opSetAt(hash, opLink);
            opGetAt(SYM_SET::nEntries)++;
            // TODO: grow the table if it's getting too crowded

            return opSym;
        }   /* OP::_StoreSymbol */


        OP OP::InternSymbolFromOp(OP opQual) {
            // pass in a string (or symbol - NOP) to get it registered or found in this SymSet
            assert(opQual.isQual() || opQual.isSym());

            Word hash;
            OP opKey = _HuntSymbol(&hash, opQual.strLoc(), opQual.strLen());
            if(opKey != NIL)
                return opKey;

            // store a new symbol if necessary, then intern and return it
            if(! opQual.isSym())
                opQual = opQualSub(opQual.strLen(), opQual, 0);

            return _StoreSymbol(hash, opQual);
        }   /* OP::InternSymbolFromOp */


        OP OP::InternSymbolStrCpy(Word bsz, const char *cp) {    // intern a Symbol, copying the C string into managed memory
            Word hash;
            OP opKey = _HuntSymbol(&hash, cp, bsz);
            if(opKey != NIL)
                return opKey;

            // store a new symbol and return it
            return _StoreSymbol(hash, opQualStrCpy(bsz, cp));
        }   /* OP::InternSymbolStrCpy */


        OP OP::InternSymbolStrRef(Word bsz, const char *cp) {    // intern a Symbol with an unmanaged string referencing a C string
            Word hash;
            OP opKey = _HuntSymbol(&hash, cp, bsz);
            if(opKey != NIL)
                return opKey;

            // store a new symbol and return it
            return _StoreSymbol(hash, opQualStrRef(bsz, cp));
        }   /* OP::InternSymbolStrRef */


        OP OP::InternSymAssoc(OP opSym) {
            // pass in a symbol to get it registered or found in this HashSet
            assert(this->isObj());
            assert(this->classOf() == SymHashClass);
            assert(opSym.isSym());

            Word hash = strHash(opSym.strLoc(), opSym.strLen()) % opGetAt(SYM_HASH::nSize).iFrom() + 1;

            OP opLink = opGetAt(SYM_HASH::opTable).opGetAt(hash);
            while(! opLink.isNil()) {
                OP opKey = opLink.opGetAt(SYM_HASH_LINK::opKey);
                if(opKey == opSym)
                    return opLink;

                // move on
                opLink = opLink.opGetAt(SYM_HASH_LINK::opLink);
            }

            // store a new Assoc and return it
            opLink = NewSymHashLink(opGetAt(SYM_HASH::opTable).opGetAt(hash), opSym);
            opGetAt(SYM_HASH::opTable).opSetAt(hash, opLink);
            opGetAt(SYM_HASH::nEntries)++;
            // TODO: grow the table if it's getting too crowded

            return opLink;
        }   /* OP::InternSymAssoc */


        OP OP::AssocGetVal() {
            return opGetAt(SYM_HASH_LINK::opVal);
        }   /* OP::AssocGetVal */


        void OP::AssocSetVal(OP op) {
            opSetAt(SYM_HASH_LINK::opVal, op);
        }   /* OP::AssocSetVal */


        Word OP::strHash() {
            if(! isQual() && ! isSym()) // TODO: error?
                return 0;

            return strHash(strLoc(), strLen());
        }   /* OP::strHash */


        Word OP::strHash(const char *cp, Word len) {
            Word i = 0, j = len;
            if(j > 10)                  // limit scan to first ten characters
                j = 10;
            while(j-- > 0) {
                i += *cp++ & 0xFF;      // add unsigned version of next char
                i *= 37;                // scale total by a nice prime number
            }

            i += len;                   // add the (untruncated) string length
            return i;
        }   /* OP::strHash */


        // get access to the ith user space word
        // TODO: prove that it's safe to yield a reference - I DOUBT IT!
        OP &OP::opGetAt(Word i) {
            getNP()->CheckAccess(i, false);
            i += getNP()->StartWord();

            return getNP()->opChilds[i];
        }   /* Node::opGetAt */


        void OP::opSetAt(Word i, OP opVal) {
            getNP()->CheckAccess(i, true);
            i += getNP()->StartWord();

            OP opOld = getNP()->opChilds[i];
            if(opOld == opVal) return;
            if(! opVal.isNil() && opVal.isRef())
                opVal.CountUp();
            getNP()->opChilds[i]  = opVal;
            if(! opOld.isNil() && opOld.isRef())
                Allocator::DeleteNp(opOld);

        }   /* Allocator::opSetAt */



        // debugging 'n that
        void OP::dump(const char *cp, bool brief) {
            if(cp)
                fprintf(stderr, "%s ", cp);
            fprintf(stderr, "*%tX: ", getNP());

            if(isNil())
                fprintf(stderr, "NIL\n");
            else if(isInt())
                fprintf(stderr, "INT %d\n", iFrom());
            else if(isQual()) {
                QualPtr qp = qpFrom();
                fprintf(stderr, "STR %s *%tX +: #%lX %.*s\n", qp->managed ? "M" : "-", qp->str, qp->nBytes, qp->nBytes, qp->str);
            }
            else if(isSym()) {
                QualPtr qp = qpFrom();
                fprintf(stderr, "SYM %s *%tX +: #%lX %.*s\n", qp->managed ? "M" : "-", qp->str, qp->nBytes, qp->nBytes, qp->str);
            }
            else {
                // isRef()
                fprintf(stderr, "%s bsz=%4d (Uwsz=%3d)", isObj() ? "PTR" : "BIN", getNP()->nh_bszNode, getNP()->wszData());
                fprintf(stderr, " _w=$%03o:%o:%c:%c:%02o:%02o:%02o:[%02o]", getNP()->nh_RC, getNP()->hdr.spare, getNP()->nh_oversize ? '+' : '-', isObj() ? 'P' : 'B', getNP()->nh_msize, getNP()->nh_extra, getNP()->nh_mofs, getNP()->nh_dims);

                if(isDim()) {
                    fprintf(stderr, " [");
                    for(int i = 0; i < getNP()->nh_dims; ++i) {
                        if(i) fprintf(stderr, " ; ");
                        Word lo = getNP()->wChilds[i * 2 + 0];
                        Word hi = getNP()->wChilds[i * 2 + 1];

                        if(lo != 1)
                            fprintf(stderr, "%d to ", lo);
                        fprintf(stderr, "%d", hi);
                    }
                    fprintf(stderr, "]");
                }

                if(isBin()) {
                    // TODO: loop over the sized object, respect msize, extra and mofs
                    Word limit = getNP()->bszData() - getNP()->nh_dims * WORD_SIZE * 2;
                    fprintf(stderr, " *%tX: limit=%d", classOf(), limit);

                    if(! brief) {
                        fprintf(stderr, " (");
                        for(Word i = 0; i < limit; ++i) {
                            if(i) fprintf(stderr, " ");
                            fprintf(stderr, "%02X", ByteAt(i));
                        }
                        fprintf(stderr, ")");
                    }
                }
                else {
                    Word i, count;
                    for(i = 0, count = getNP()->LimitWord() - getNP()->StartWord(); i < count; ++i) {
                        OP x = opGetAt(i);
                        int repeats = 1;
                        
                        // try to compactly show runs of repeated words, but not starting from the first...
                        if(i > 0)
                            while(i + repeats < count && opGetAt(i + repeats) == x)
                                ++repeats;

                        // separate the class pointer slightly
                        if(i == 1) fprintf(stderr, ":");
                        if(brief && i) break;

                             if(x.isNil())  fprintf(stderr, " NIL");
                        else if(x.isRef())  fprintf(stderr, " *%tX", x.i);
                        else if(x.isInt())  fprintf(stderr, " %d", x.iFrom());
                        else if(x.isQual()) fprintf(stderr, " \"*%tX\"", x.qpFrom());
                        else if(x.isSym())  fprintf(stderr, " :*%tX", x.qpFrom());

                        if(repeats >= 3) {
                            i += repeats - 1;
                            fprintf(stderr, "[x%d]", repeats);
                        }
                    }
                }

                fprintf(stderr, "\n");
            }
        }   /* dump */

        
        void OP::dumpRanges(const char *cp) {
            CHATTER("    ");
            if(cp) CHATTER("%s: ", cp);
            CHATTER("%X %X %X %X\n", getNP()->StartWord(), getNP()->LimitWord(), getNP()->StartPtr(), getNP()->LimitPtr());
        }


        Char &OP::CharAt(Word i) {
            return CharPtr(getNP()->byChilds + getNP()->nh_dims * 2 * WORD_SIZE + getNP()->nh_mofs)[i];
        }   /* OP::CharAt */


        Int &OP::IntAt(Word i) {
            return IntPtr(getNP()->byChilds + getNP()->nh_dims * 2 * WORD_SIZE + getNP()->nh_mofs)[i];
        }   /* OP::IntAt */


        Int16 &OP::Int16at(Word i) {
            return Int16Ptr(getNP()->byChilds + getNP()->nh_dims * 2 * WORD_SIZE + getNP()->nh_mofs)[i];
        }   /* OP::Int16at */


        Int32 &OP::Int32at(Word i) {
            return Int32Ptr(getNP()->byChilds + getNP()->nh_dims * 2 * WORD_SIZE + getNP()->nh_mofs)[i];
        }   /* OP::Int32at */


        Int64 &OP::Int64at(Word i) {
            return Int64Ptr(getNP()->byChilds + getNP()->nh_dims * 2 * WORD_SIZE + getNP()->nh_mofs)[i];
        }   /* OP::Int64at */


        Byte &OP::ByteAt(Word i) {
            return BytePtr(getNP()->byChilds + getNP()->nh_dims * 2 * WORD_SIZE + getNP()->nh_mofs)[i];
        }   /* OP::ByteAt */


        Word &OP::WordAt(Word i) {
            return WordPtr(getNP()->byChilds + getNP()->nh_dims * 2 * WORD_SIZE + getNP()->nh_mofs)[i];
        }   /* OP::WordAt */


        Word16 &OP::Word16at(Word i) {
            return Word16Ptr(getNP()->byChilds + getNP()->nh_dims * 2 * WORD_SIZE + getNP()->nh_mofs)[i];
        }   /* OP::Word16at */


        Word32 &OP::Word32at(Word i) {
            return Word32Ptr(getNP()->byChilds + getNP()->nh_dims * 2 * WORD_SIZE + getNP()->nh_mofs)[i];
        }   /* OP::Word32at */


        Word64 &OP::Word64at(Word i) {
            return Word64Ptr(getNP()->byChilds + getNP()->nh_dims * 2 * WORD_SIZE + getNP()->nh_mofs)[i];
        }   /* OP::Word64at */



        Float32 &OP::Float32at(Word i) {
            return ((Float32 *) (getNP()->byChilds + getNP()->nh_dims * 2 * WORD_SIZE + getNP()->nh_mofs))[i];
        }   /* OP::Float32at */


        Float64 &OP::Float64at(Word i) {
            return ((Float64 *) (getNP()->byChilds + getNP()->nh_dims * 2 * WORD_SIZE + getNP()->nh_mofs))[i];
        }   /* OP::Float64at */


        FloatLD &OP::FloatLDat(Word i) {
            return ((FloatLD *) (getNP()->byChilds + getNP()->nh_dims * 2 * WORD_SIZE + getNP()->nh_mofs))[i];
        }   /* OP::FloatLDat */

    };  /* namespace Ice9::Memory */
};  /* namespace Ice9 */

