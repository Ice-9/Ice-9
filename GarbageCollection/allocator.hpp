
# ifndef _ALLOCATOR_H_
#  define _ALLOCATOR_H_

namespace Ice9 {
    namespace Memory {
        class Allocator {
            friend int main(int argc, char *argv[]);
            friend class Node;
            friend class OP;

            typedef void (*markerCallback)(OP op);
            typedef void (*markerFtn)(markerCallback hp);

            // allows call-backs
            static void Marker(OP op);

            int checkNo;                        // serial number of calls to CheckArena

            Extent::Ptr extents;                // private list of extents for this allocator
            static void ExpandArena(Word bsz=0);

            OP opRoots[MAX_ROOTS];              // private list of all root objects for this Allocator for GC purposes
            int cntRoots;

            // HMMM: should all threads share the same markers??
            markerFtn rootMarkers[MAX_ROOT_MARKERS];
            int cntRootMarkers;

            NodePtr npFrees[FREE_QUICKIES];     // freelist cache of common small allocation sizes
            Node nBase;                         // sentinel for circular list of free blocks
            NodePtr npAllocp;                   // current position of allocation in free list
            NodePtr npChaser;                   // the node before the current when allocating/freeing

            QualifierExtent::Ptr qepList;
            QualPtr qpFree;
            Word nQual;                         // count of allocated Qualifiers

            Word bszStrWanted;
            CharPtr strBase, strFree, strEnd;
            QualPtr *qppBase, *qppFree, *qppEnd;   // addresses of slots in objects wot point to Qualifiers
            Word szPost;        // allocated size of qualifier table

            static QualPtr AllocQp(Word bsz, CharPtr str);
            static void FreeQp(QualPtr qp);
            static CharPtr StrReserve(Word bsz);

            static void ZeroRefCounts();
            static void MarkAccessibleObjects(OP);
            static void RectifyCountsAndDeallocateGarbage();
            static void PostQual(OP op);
            static void StringsReclaim();

            static void PlainFree(NodePtr);
            static void FreeListInsert(NodePtr np, NodePtr npPrev);
            static void DeferredDown(NodePtr);
            static void DeleteNp(OP);
            static void FreeNp(NodePtr);
            static NodePtr npAllocate(Word bsz);

            // NodePtr handling
            static void RegisterRoot(OP);
            static void DeregisterRoot(OP);

            static OP opNewObj(OP opClass, Word align, Word szEa, Word dims, std::va_list ap);
            static OP opNewStr(Word bsz, const char *cp, bool managed);

            static __thread Ice9::Memory::Allocator alloc;

          public:
            // Arena handling
            static void Init();
            static void MakeSpaces();
            static void Destroy();
            static void CheckArena(const char *const message);
            static void ReclaimInaccessibleObjects();          // only made public so that collections can be forced. Don't do that...

            static void RegisterRootMarker(markerFtn);
            static void DeregisterRootMarker(markerFtn);

        };  /* class Allocator */

    };  /* namespace Ice9::Memory */
};  /* namespace Ice9 */

# endif

