
# include   "standard.hpp"
# include   "memory9.hpp"


namespace Ice9 {
    namespace Memory {
        const NodePtr Node::NIL = NodePtr(0);

        void Node::CheckAccess(Word i, bool update) {
            char const *ap = update ? "Update" : "Access";

            if(! this)
                ABORT("Attempt to %s member of NIL object\n", ap);

            if(! nh_isptrs)
                ABORT("Attempt to %s member of a non-pointer object *%tX\n", ap, this);

            if(i < 0)
                ABORT("Attempt to %s object *%tX before the child array at %d\n", ap, this, i);

            if(i >= LimitWord() - StartWord())
                ABORT("Attempt to %s object *%tX beyond size of child array at %d\n", ap, this, i);

        }   /* Node::CheckAccess */

    };  /* namespace Ice9::Memory */
};  /* namespace Ice9 */

