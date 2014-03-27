
# ifndef _COREDEFS_H_
#  define _COREDEFS_H_class


// expansion size in bytes - MUST be a multiple of ALIGN_SIZE_BYTES - check it manually
# ifndef HEAP_EXPAND
# define HEAP_EXPAND        (32 * 1024 * Ice9::Memory::WORD_SIZE)
# endif

// expansion size in bytes - MUST be a multiple of ALIGN_SIZE_BYTES - check it manually
# ifndef STR_EXPAND
# define STR_EXPAND         (16 * 1024 * Ice9::Memory::WORD_SIZE)
# endif

// the 'root' objects are reserved ObjID's which have compile-time constant values in the Object Table.
// things like Object, Nil, True, False, the classes representing primitives, etc etc etc
# ifndef MAX_ROOTS
# define MAX_ROOTS          20
# endif

# ifndef MAX_ROOT_MARKERS
# define MAX_ROOT_MARKERS   20
# endif

// size of freelist cache of common small allocation sizes - do stats and see what's useful
// note: having freelists of small sizes interferes with coalescing.
# ifndef FREE_QUICKIES
# define FREE_QUICKIES      10
# endif

// PICK ONE!
# define CHATTY(x...)       x
//define CHATTY(x...)
# define CHATTER(x...)      CHATTY(fprintf(stderr, x))

# define ABORT(x...)        Ice9::Abort(__FILE__, __LINE__, x);


namespace Ice9 {

    inline void Abort(const char *const fname, int line, const char *const fmt, ...) {
       va_list ap;
        if(fname)
            fprintf(stderr, "%s[%d]: ", fname, line);

        va_start(ap, fmt);
        vfprintf(stderr, fmt, ap);
        va_end(ap);
        abort();
    }   /* Abort */


    namespace Memory {
        class Qualifier;    typedef Qualifier *QualPtr;
        class Node;         typedef Node *NodePtr;

        // TODO:check out std::numeric_limits (or std::type_traits or similar) to
        // try to track down traits for alignment. See comment below about FloatLD
        union __align_u {   // guarantees all structures align with worst case.
            int i;          // straw poll - what IS the worst case?
            long int li;
            long long int lli;
            Float32 f;
            Float64 d;
         // FloatLD ld;     // This beast blows out the sizes - specifically,
        };                  // messing up Memory::Qualifier. Need a smarter way to
                            // get alignment. Is there a standard #define declaring
                            // worst-case alignment without causing bulk?

        // FOR INTEREST: the worst case mentioned above might affect the size of
        // things like qualifiers (for example). e.g. a qualifier is supposed to be
        // 2 machine words - a natural pointer and a same-sized native integer for
        // string length. If that pair of words is smaller than the worst alignment
        // (eg a long double maybe) then memory consumption will blow out.
        // NOTE: as you'll notice above, the decision to keep FloatLD out of the
        // alignment calculations has been taken. On Intel, a mis-aligned long
        // double still works but may be a bit slower to transfer. Other chips are
        // not so forgiving...


        struct __align {
            Byte x;         // this in first place means the next member must be at non-zero offset
            __align_u u;    // thus, this guy is at align-size offset. We can measure that with a macro ALIGN_SIZE_BYTES
        };

        // alignment needed for the worst case native type
        // used to set the size blocking factor of allocations.
        # define    ALIGN_SIZE_BYTES            ((int) (uintptr_t) &((Ice9::Memory::__align *)0)->u)

        // tools to generate alignment needs of all native types
        # define    ALIGN_PREP(Name, Type)      struct __Align__##Name { Byte x; union { Type obj; } u; }
        # define    ALIGN(Name)                 ((int) (uintptr_t) &((__Align__##Name *)0)->u)

        ALIGN_PREP(Word, Word);
        ALIGN_PREP(int, int);
        ALIGN_PREP(long_int, long int);
        ALIGN_PREP(long_long_int, long long int);
        ALIGN_PREP(Float32, Float32);
        ALIGN_PREP(Float64, Float64);
        ALIGN_PREP(FloatLD, FloatLD);

        const int align_Word          = ALIGN(Word);
        const int align_int           = ALIGN(int);
        const int align_long_int      = ALIGN(long_int);
        const int align_long_long_int = ALIGN(long_long_int);
        const int align_Float32       = ALIGN(Float32);
        const int align_Float64       = ALIGN(Float64);
        const int align_FloatLD       = ALIGN(FloatLD);


        // TODO: the following probably should be replaced by relying on the msize data member of arrayed objects
        //
        const int CHAR_SIZE  = sizeof(Char);
        const int INT_SIZE   = sizeof(Int);
        const int INT16_SIZE = sizeof(Int16);
        const int INT32_SIZE = sizeof(Int32);
        const int INT64_SIZE = sizeof(Int64);

        const int BYTE_SIZE   = sizeof(Byte);
        const int WORD_SIZE   = sizeof(Word);
        const int WORD16_SIZE = sizeof(Word16);
        const int WORD32_SIZE = sizeof(Word32);
        const int WORD64_SIZE = sizeof(Word64);

        const int FLOAT32_SIZE = sizeof(Float32);
        const int FLOAT64_SIZE = sizeof(Float64);
        const int FLOATLD_SIZE = sizeof(FloatLD);

    };  /* namespace Ice9::Memory */
};  /* namespace Ice9 */

# endif

