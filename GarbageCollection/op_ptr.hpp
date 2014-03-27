
// Locked-down ptr that can't interact with other types.
// By design: Does not play well with other types.
// Because it's an OP, it's two lowest bits are tagged.
// Debugging must show it to have tag-bits == 00 or die.
// Since an OP is the only one that does not have the two low bits hacked,
// the code does not need to fiddle around with bits or shifts.
// However, it's important that all Words are aligned to at least 4 bytes
// so that the low two bits will be zero for these pointers.

# ifndef OP_PTR_HPP
#  define OP_PTR_HPP

namespace Ice9 {
    enum OPPtrType {
        OP_SHIFT = 2,
        OP_MASK  = (1 << OP_SHIFT) - 1,
        OP_REF   = 0x00,                // pointer to node
        OP_INT   = 0x01,                // (nearly) native integer
        OP_QUAL  = 0x02,                // pointer to Qualifier - a leafy thingy, points into the string space only
        OP_SYM   = 0x03,                // pointer to Qualifier - a leafy thingy, points into the string space only
    };

    namespace Memory {

        class OP;
        extern const OP NIL;

        class OP {
            friend int main(int argc, char *argv[]);
            // friend class Allocator;

          public:
            static OP ClassClass;
            static OP ObjectClass;
            static OP NilClass;
            static OP IntClass;
            static OP QualClass;
            static OP SymbolClass;
            static OP BooleanClass;
            static OP SymSetClass;
            static OP SymHashClass;
            static OP SymSetLinkClass;
            static OP SymHashLinkClass;

            static OP SymbolsSet;
            static OP GlobalsHash;


          private:
            union {
                NodePtr _np;
                Int i;
            };

            // TODO: the place to adjust when offsetted pointers are used
            inline NodePtr getNP();
            inline void setNP(NodePtr np);

            // memory management converters - from OP to ordinary C types
          public:
            inline Int iFrom();
            inline Int IntPart();
            inline NodePtr npFrom();
            inline QualPtr qpFrom();

            inline explicit operator VoidPtr() { return (void *) i; }      // TODO: GET RID OF THIS!

            // should not be published directly...
            inline Word bszData();
            inline Word wszData();

            inline Word LimitPtr();
            inline Word LimitWord();
            inline Word StartPtr();
            inline Word StartWord();

            inline void CountDown();
            inline void CountUp();

            inline void CDown();
            inline void CUp();

            inline void registerRoot();
            inline void deregisterRoot();

            static void registerClassClass(OP op);
            static void registerNilClass(OP op);
            static void registerObjectClass(OP op);
            static void registerIntClass(OP op);
            static void registerQualClass(OP op);
            static void registerSymbolClass(OP op);
            static void registerBooleanClass(OP op);
            static void registerSymSetClass(OP op);
            static void registerSymHashClass(OP op);
            static void registerSymSetLinkClass(OP op);
            static void registerSymHashLinkClass(OP op);

            static void StandardClasses();
            static void StandardObjects();

            static OP NewSymSet();
            static OP NewSymHash();         // the code looks the same so far, but when it uses real methods....
            static OP NewSymSetLink(OP opLink, OP opKey);
            static OP NewSymHashLink(OP opLink, OP opKey, OP opVal = NIL);

            // NOTE: intern here means to see if it's already interned, returning that or creating it for the first time
            OP _HuntSymbol(Word *hash, const char *cp, Word len);
            OP _StoreSymbol(Word hash, OP opSym);
            OP InternSymbolFromOp(OP opQual);                   // intern a Symbol, referencing another Qual, with the managed flag copied
            OP InternSymbolStrCpy(Word bsz, const char *cp);    // intern a Symbol, copying the C string into managed memory
            OP InternSymbolStrRef(Word bsz, const char *cp);    // intern a Symbol with an unmanaged string referencing a C string

            OP InternSymAssoc(OP opSym);    // using a Symbol key, intern in an assocation.


            OP AssocGetVal();
            void AssocSetVal(OP op);

            bool operator == (OP r) { return i == r.i; }
            bool operator != (OP r) { return i != r.i; }

            // type predicates
            inline bool isNil();
            inline bool isRef();    // can't distinquish NIL - hence you need to call isNil where applicable
            inline bool isInt();
            inline bool isQual();
            inline bool isSym();
            inline bool isObj();    // detects ! isNil() && isRef()
            inline bool isDim();
            inline bool isBin();


            // memory management converters - from ordinary C types to OP
            friend OP opRaw(Int i);             // only used to initialize NIL - try to do better...
            friend OP opFromRef(NodePtr np);    // TODO:should not be published?
            friend OP opFromInt(Int i);
            friend OP opFromQual(QualPtr qp);   // TODO:should not be published?


            // qualifier features
            static inline OP opQualNew(Word bsz);                       // allocate a Qual with a managed string, contents uninitialized
            static inline OP opQualStrCpy(Word bsz, const char *cp);    // allocate a Qual, copying the C string into managed memory
            static inline OP opQualStrRef(Word bsz, const char *cp);    // allocate a Qual with an unmanaged string referencing a C string
            static inline OP opQualSub(Word bsz, OP str, Word ofs);     // allocate a Qual referencing a substring of another Qual, with the managed flag copied

            inline CharPtr strLoc();        // get the address of the content string
            inline Word strLen();           // get the length of the content string
            Word strHash();                 // get the hash of a Qual or Symbol
            static Word strHash(const char *cp, Word len);

            // Scalar MI Object features
            // pass align=0,szEa=0 for MI object
            // pass them non-zero (1+) for binary object
            static inline OP opObjectNew(OP opClass, Word align, Word szEa, Word nItems);

            // Dimensioned Object features
            static inline OP opArrayNew(OP opClass, Word align, Word szEa, Word dims, ...);
            static inline OP opArrayNewV(OP opClass, Word align, Word szEa, Word dims, std::va_list ap);
            // NOTE: the params encode for array of OP vs. array of binaries.
            // DOES NOT support array of expanded objects - use of offsetted
            // pointers resolved through the class vtable kinda screws the
            // ability to point at individual objects. Perhaps it could work
            // when Value types are working?


            friend OP operator + (const OP l, const OP r);
            friend OP operator - (const OP l, const OP r);
            friend OP operator * (const OP l, const OP r);
            friend OP operator / (const OP l, const OP r);
            friend OP operator % (const OP l, const OP r);

            friend OP operator & (const OP l, const OP r);
            friend OP operator | (const OP l, const OP r);
            friend OP operator ^ (const OP l, const OP r);

            friend OP operator << (const OP l, const OP r);
            friend OP operator >> (const OP l, const OP r);

            friend OP operator << (const OP l, const Int r);
            friend OP operator >> (const OP l, const Int r);

            friend OP &operator ++ (OP &I);       // Prefix
            friend OP  operator ++ (OP &I, int);  // Postfix
            friend OP &operator -- (OP &I);       // Prefix
            friend OP  operator -- (OP &I, int);  // Postfix

            inline void operator += (const OP I) { i += I.i & ~ OP_MASK; }
            inline void operator -= (const OP I) { i -= I.i & ~ OP_MASK; }
            inline void operator *= (const OP I) { i  = (((i >> OP_SHIFT) * (I.i >> OP_SHIFT)) << OP_SHIFT) | OP_INT; }
            inline void operator /= (const OP I) { i  = (((i >> OP_SHIFT) / (I.i >> OP_SHIFT)) << OP_SHIFT) | OP_INT; }
            inline void operator %= (const OP I) { i  = (((i >> OP_SHIFT) % (I.i >> OP_SHIFT)) << OP_SHIFT) | OP_INT; }
            inline void operator &= (const OP I) { i &= I.i; }
            inline void operator |= (const OP I) { i |= I.i; }
            inline void operator ^= (const OP I) { i ^= I.i ^ OP_INT; }

            inline void operator <<= (const OP I) { i  = ((i & ~ OP_MASK) << (I.i >> OP_SHIFT)) | OP_INT; }
            inline void operator >>= (const OP I) { i  = ((i & ~ OP_MASK) >> (I.i >> OP_SHIFT)) | OP_INT; }

            inline void operator <<= (const Int _i) { i  = ((i & ~ OP_MASK) << _i) | OP_INT; }
            inline void operator >>= (const Int _i) { i  = ((i & ~ OP_MASK) >> _i) | OP_INT; }

            inline OP operator + () { return *this; }
            inline OP operator - () { OP result; result.i = 2 - i; return result; }
            inline OP operator ~ () { OP result; result.i = (~ i) ^ OP_MASK; return result; }

            void dump(const char *cp = NULL, bool brief = false);
            void dumpRanges(const char *cp);

            inline OP classOf();
            // TODO: prove that it's safe to yield a reference - I DOUBT IT!
            OP &opGetAt(Word ofs);
            void opSetAt(Word ofs, OP opv);


            // low-level signed accessor utilities - callers need to know they have the correct type - ie correct use for a ptr qual int or uchar etc
            Char   &CharAt  (Word);
            Int    &IntAt   (Word);
            Int16  &Int16at (Word);
            Int32  &Int32at (Word);
            Int64  &Int64at (Word);

            Byte   &ByteAt  (Word);
            Word   &WordAt  (Word);
            Word16 &Word16at(Word);
            Word32 &Word32at(Word);
            Word64 &Word64at(Word);

            Float32 &Float32at(Word);
            Float64 &Float64at(Word);
            FloatLD &FloatLDat(Word);
        };  // class OP

        // constants to access members of Internal Objects
        class SYM_SET       { public: enum { opClass, nSize, nEntries, opTable, SIZE }; };
        class SYM_HASH      { public: enum { opClass, nSize, nEntries, opTable, SIZE }; };
        class SYM_SET_LINK  { public: enum { opClass, opLink, opKey,            SIZE }; };
        class SYM_HASH_LINK { public: enum { opClass, opLink, opKey, opVal,     SIZE }; };

        // memory management converters - from ordinary C types to OP
        inline OP opRaw(Int i);
        inline OP opFromRef(NodePtr np);
        inline OP opFromInt(Int i);
        inline OP opFromQual(QualPtr qp);


        inline OP operator + (OP l, OP r) {
            OP result;
            result.i = l.i + (r.i & ~ OP_MASK);
            return result;
        }

        inline OP operator - (OP l, OP r) {
            OP result;
            result.i = l.i - (r.i & ~ OP_MASK);
            return result;
        }

        inline OP operator * (OP l, OP r) {
            OP result;
            result.i = (l.i & ~ OP_MASK) * (r.i >> OP_SHIFT);
            return result;
        }

        inline OP operator / (OP l, OP r) {
            OP result;
            result.i = (l.i >> OP_SHIFT) / (r.i >> OP_SHIFT);
            return result;
        }

        inline OP operator % (OP l, OP r) {
            OP result;
            result.i = (l.i >> OP_SHIFT) % (r.i >> OP_SHIFT);
            return result;
        }

        inline OP operator & (OP l, OP r) {
            OP result;
            result.i = l.i & r.i;
            return result;
        }

        inline OP operator | (OP l, OP r) {
            OP result;
            result.i = l.i | r.i;
            return result;
        }

        inline OP operator ^ (OP l, OP r) {
            OP result;
            result.i = l.i ^ r.i ^ OP_INT;
            return result;
        }


        inline OP operator << (const OP l, const OP r) {
            OP result;
            result.i = (l.i >> OP_SHIFT) << (r.i >> OP_SHIFT);
            return result;
        }

        inline OP operator >> (const OP l, const OP r) {
            OP result;
            result.i = (l.i >> OP_SHIFT) >> (r.i >> OP_SHIFT);
            return result;
        }


        inline OP operator << (const OP l, const Int r) {
            OP result;
            result.i = (l.i >> OP_SHIFT) << r;
            return result;
        }

        inline OP operator >> (const OP l, const Int r) {
            OP result;
            result.i = (l.i >> OP_SHIFT) >> r;
            return result;
        }


        inline OP &operator ++ (OP &I) {      // Prefix
            I.i += (1 << OP_SHIFT);
            return I;
        }

        inline OP operator ++ (OP &I, int) {  // Postfix
            OP result = I;
            ++I.i += (1 << OP_SHIFT);
            return result;
        }


        inline OP &operator -- (OP &I) {      // Prefix
            I.i -= (1 << OP_SHIFT);
            return I;
        }

        inline OP operator -- (OP &I, int) {  // Postfix
            OP result = I;
            I.i -= (1 << OP_SHIFT);
            return result;
        }

    };  /* namespace Ice9::Memory */
};  /* namespace Ice9 */

# endif

