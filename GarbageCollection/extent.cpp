
# include   "standard.hpp"
# include   "memory9.hpp"

namespace Ice9 {
    namespace Memory {

        Extent::Ptr Extent::epAlloc(Word bsz) {
            Extent::Ptr ep = (Extent::Ptr) malloc(bsz);
            if(! ep)
                ABORT("Memory exhausted\n");
            memset(ep, 0, bsz);

            return ep;
        }   /* Extent::epAlloc */

    };  /* namespace Ice9::Memory */
};  /* namespace Ice9 */

