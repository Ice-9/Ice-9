
# ifndef _QUALIFIER_H_
#  define _QUALIFIER_H_

namespace Ice9 {
    namespace Memory {

        // Smallest possible record suitable as a string qualifier - 2 machine words.
        // A NodePtr with the OP_QUAL bits set acually points to this - just zero the magic bits.
        // Allocated in blocks and chucked on their own free list.
        //
        class Qualifier {
            friend class QualifierExtent;
            friend class Allocator;
            friend class Node;
            friend class OP;

            union {
                struct {
                    // TODO: does this thing need to be a struct with a full word size for when on the free list?
                    Word marked : 1,
                         managed : 1,
                         nBytes : 30;       // length of string being described

                    union {
                        QualPtr qpNext;     // when on the free list
                        CharPtr str;        // when it's pointing to a live string
                    };
                };
            };
        };

        // marks a qualifier during GC
        # define    MARK_QUAL       (UINTPTR_MAX ^ (UINTPTR_MAX >> 1))


        // Qualifiers are small and all the same size so they live in their own free list
        class QualifierExtent {
          public:
            typedef QualifierExtent *Ptr;

          private:
            friend class Allocator;

            QualifierExtent::Ptr qepNext;     // so the Allocator can keep track of all the extents
            Word nq;
            Qualifier qArray[0];

            static void AllocateQe(Word nQualifiers, QualifierExtent::Ptr &qepList, QualPtr &qpFree);
        };

    };  /* namespace Ice9::Memory */
};  /* namespace Ice9 */

# endif

