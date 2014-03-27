
/* A bucket for inlines that can't reasonably be placed into header files
 * due to forward references between the classes. Probably not a good idea
 * to just stuff all inlines into here - keep them with their class if at
 * all possible.
 */

# ifndef _INLINES_H_
#  define _INLINES_H_

namespace Ice9 {
    namespace Memory {

        inline Word Node::bszData() { assert(this != NULL); return nh_bszNode - NHDR_SIZE - nh_extra - nh_mofs; }
        inline Word Node::wszData() { assert(this != NULL); return (nh_bszNode - NHDR_SIZE - nh_extra) / WORD_SIZE - nh_oversize; }

        inline Word Node::StartWord() { assert(this != NULL); return nh_dims * 2; }
        inline Word Node::StartPtr()  { assert(this != NULL); return nh_dims * 2; }
        inline Word Node::LimitWord() { assert(this != NULL); return wszData(); }
        inline Word Node::LimitPtr()  { assert(this != NULL); if(nh_isptrs) return wszData(); return nh_dims * 2 + 1; }   // TODO: maybe callers shouldn't call if it's not pointered...

        inline NodePtr npAddBsz(NodePtr np, Word bsz) {
            // adds byte-count to the address of a node
            return NodePtr(BytePtr(np) + bsz);
        }

        // TODO: the place to adjust when offsetted pointers are used
        inline NodePtr OP::getNP()        { return _np ? _np - 1 : _np; }
        inline void OP::setNP(NodePtr np) { _np = np ? np + 1 : np; }

        inline Int OP::iFrom()        { return (i & ~ OP_INT) >> OP_SHIFT; }
        inline Int OP::IntPart()      { return i; }
        inline NodePtr OP::npFrom()   { return getNP(); }
        inline QualPtr OP::qpFrom()   { return QualPtr(i & ~ (OP_QUAL | OP_SYM)); }

        inline OP opRaw(Int i)           { OP result; result.i = i;                        return result; }
        inline OP opFromRef(NodePtr np)  { OP result; result.setNP(np);                    return result; }
        inline OP opFromInt(Int i)       { OP result; result.i = (i << OP_SHIFT) | OP_INT; return result; }
        inline OP opFromQual(QualPtr qp) { OP result; result.i = Word(qp) | OP_QUAL;       return result; }

        inline bool OP::isNil()  { return getNP() == 0; }
        inline bool OP::isRef()  { return (i & OP_MASK) == OP_REF;  }
        inline bool OP::isInt()  { return (i & OP_MASK) == OP_INT;  }       // implies not NULL
        inline bool OP::isQual() { return (i & OP_MASK) == OP_QUAL; }       // implies not NULL
        inline bool OP::isSym()  { return (i & OP_MASK) == OP_SYM;  }       // implies not NULL
        inline bool OP::isObj()  { assert(_np != NULL); return getNP()->nh_isptrs;   }
        inline bool OP::isBin()  { assert(_np != NULL); return ! getNP()->nh_isptrs; }
        inline bool OP::isDim()  { assert(_np != NULL); return getNP()->nh_dims;     }

        inline Word OP::bszData() { assert(_np != NULL); return getNP()->bszData(); }
        inline Word OP::wszData() { assert(_np != NULL); return getNP()->wszData(); }

        inline Word OP::LimitPtr()  { assert(_np != NULL); return getNP()->LimitPtr();  }
        inline Word OP::LimitWord() { assert(_np != NULL); return getNP()->LimitWord(); }
        inline Word OP::StartPtr()  { assert(_np != NULL); return getNP()->StartPtr();  }
        inline Word OP::StartWord() { assert(_np != NULL); return getNP()->StartWord(); }

        inline void OP::CountDown() { assert(_np != NULL); getNP()->CountDown(); }
        inline void OP::CountUp()   { assert(_np != NULL); getNP()->CountUp();   }

        inline void OP::CDown() { if(! isNil() && isRef()) getNP()->CountDown(); }
        inline void OP::CUp()   { if(! isNil() && isRef()) getNP()->CountUp();   }

        inline void OP::registerRoot()   { assert(this != NULL); Allocator::RegisterRoot(*this);   }
        inline void OP::deregisterRoot() { assert(this != NULL); Allocator::DeregisterRoot(*this); }

        // qualifier related features
        inline OP OP::opQualStrCpy(Word bsz, const char *cp) {
            assert(bsz >= 0);
            assert(cp != NULL);
            OP result = Allocator::opNewStr(bsz, NULL, true);
            strncpy(result.strLoc(), cp, bsz);
            return result;
        }

        inline OP OP::opQualNew(Word bsz) {
            assert(bsz >= 0);
            return Allocator::opNewStr(bsz, NULL, true);
        }

        inline OP OP::opQualStrRef(Word bsz, const char *cp) {
            assert(bsz >= 0);
            assert(cp != NULL);
            return Allocator::opNewStr(bsz, cp, false);
        }

        inline OP OP::opQualSub(Word bsz, OP str, Word ofs) {
            assert(bsz >= 0);
            assert(str.isQual() || str.isSym());
            assert(ofs >= 0);
            assert(ofs + bsz < str.strLen());
            return Allocator::opNewStr(bsz, str.strLoc() + ofs, str.qpFrom()->managed);
        }


        inline CharPtr OP::strLoc() {
            assert(isQual() || isSym());
            return qpFrom()->str;
        }

        inline Word OP::strLen() {
            assert(isQual() || isSym());
            return qpFrom()->nBytes;
        }


        inline OP OP::classOf() {
            if(isNil())  return OP::NilClass;
            if(isInt())  return OP::IntClass;
            if(isQual()) return OP::QualClass;
            if(isSym())  return OP::SymbolClass;
            assert(isRef());

            return getNP()->opChilds[getNP()->StartWord()];
        }   /* Node::classOf */


        // if allocating a pointered object (align=0) you must include an nItem for the class(es)
        // if allocating a binary object (align>0) you must not include count for the class
        inline OP OP::opObjectNew(OP opClass, Word align, Word szEa, Word nItems) {
            return Allocator::opNewObj(opClass, align, szEa, nItems, NULL);
        }


        // when allocating an array, do not include an nItem for the class.
        // looks like arrays can only have one class??? bummer... NEEDS THOUGHT!
        // arrays should not be classed?
        inline OP OP::opArrayNewV(OP opClass, Word align, Word szEa, Word dims, std::va_list ap) {
            return Allocator::opNewObj(opClass, align, szEa, dims, ap);
        }


        // when allocating an array, do not include an nItem for the class.
        // looks like arrays can only have one class??? bummer... NEEDS THOUGHT!
        // arrays should not be classed?
        inline OP OP::opArrayNew(OP opClass, Word align, Word szEa, Word dims, ...) {
            std::va_list ap;

            va_start(ap, dims);
            OP result = Allocator::opNewObj(opClass, align, szEa, dims, ap);
            va_end(ap);
            return result;
        }

    };  /* namespace Ice9::Memory */
};  /* namespace Ice9 */

# endif

