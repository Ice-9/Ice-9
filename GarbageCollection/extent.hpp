
# ifndef _EXTENT_H_
#  define _EXTENT_H_


namespace Ice9 {
    namespace Memory {

        class Extent {
          public:
            typedef Extent *Ptr;

          private:
            friend class Allocator;

            Word bsz;
            Extent::Ptr epNext;
            NodePtr wArray[0];

          public:
            static Extent::Ptr epAlloc(Word bsz);

        };  /* Extent:: */

    };  /* namespace Ice9::Memory */
};  /* namespace Ice9 */

# endif

