
# define    __STDC_LIMIT_MACROS
# define    _FILE_OFFSET_BITS       64

# include       <stdio.h>
# include       <endian.h>
# include       <ctype.h>
# include       <string.h>
# include       <stdlib.h>
# include       <stdint.h>
# include       <unistd.h>
# include       <limits.h>
# include       <float.h>
# include       <math.h>
# include       <pthread.h>
# include       <assert.h>
# include       <signal.h>
# include       <setjmp.h>
# include       <time.h>

# include       <sys/types.h>
# include       <sys/stat.h>
# include       <fcntl.h>

# include       <readline/readline.h>
# include       <readline/history.h>
# include       <pwd.h>


# include       "errors.hpp"
# include       "rwlock.hpp"
# include       "ice9types.hpp"
# include       "standard.hpp"
# include       "memory9.hpp"


# define    SIZE_LINEBUF        1024
# define    SIZE_RETURN_STACK   10000
# define    SIZE_DATA_STACK     10000
# define    SIZE_OBJECT_STACK   1000
# define    SIZE_FLOAT_STACK    100
# define    SIZE_DICTIONARY     50000
# define    SIZE_DICT_TOKENS    5000
# define    SIZE_SEARCH_ORDER   8
# define    SIZE_PADBUF         250
# define    SIZE_PNOBUF         250
# define    SIZE_BLK_BUFFER     1024
# define    NUM_BLK_BUFFERS     100

# define    FALSE       0
# define    TRUE        -1

# define    COND(x)     ((x) ? TRUE : FALSE)


# define    XSTR(X)             #X
# define    STR(X)              XSTR(X)

# define    THREAD_VAR          __thread

# if -10 / 7 == -1
    // native symmetrical integer division
#  define   DIV_SYMMETRICAL     1
#  define   DIV_FLOORING        0
# else
    // native flooring integer division
#  define   DIV_SYMMETRICAL     0
#  define   DIV_FLOORING        1
# endif



# define    TIL_FILE_RO         0
# define    TIL_FILE_WO         1
# define    TIL_FILE_RW         2

# define    TIL_FILE_BIN        4

# define    TIL_FILE_RO_STR     "r"
# define    TIL_FILE_WO_STR     "w"
# define    TIL_FILE_RW_STR     "r+"


# define    MAX_LOCALS          16
struct LocalsDef {
    // Cell offset;     // offset from pointer to RS
    Char *cp;           // points to name of identifier, stralloc'd
    Cell nLength;
    bool initialized;
};


# if __WORDSIZE == 16
#  if __BYTE_ORDER == __BIG_ENDIAN
#   define  DICT_STRING(c1, c2)         Cell((((c1) << 8) & 0xFF00) | (c2))
#  endif

#  if __BYTE_ORDER == __LITTLE_ENDIAN
#   define  DICT_STRING(c1, c2)         Cell((c1) | (((c2) << 8) & 0xFF00))
#  endif

#  if __BYTE_ORDER == __PDP_ENDIAN
DIE!
#  endif
# endif

# if __WORDSIZE == 32 || __WORDSIZE == 64
#  if  __BYTE_ORDER == __BIG_ENDIAN
#   define  DICT_STRING(c1,c2,c3,c4)   Cell((((c1) << 24) & 0xFF000000) | (((c2) << 16) & 0x00FF0000) | (((c3) << 8) & 0x0000FF00) | (c4))
#  endif

#  if  __BYTE_ORDER == __LITTLE_ENDIAN
#   define  DICT_STRING(c1,c2,c3,c4)   Cell((c1) | (((c2) << 8) & 0x0000FF00) | (((c3) << 16) & 0x00FF0000) | (((c4) << 24) & 0xFF000000))
#  endif

#  if __BYTE_ORDER == __PDP_ENDIAN
DIE!
#  endif
# endif

# define    IMMEDIATE_BIT       HIGH_BIT



template <int Size> class CountedString {
    Cell len;
    Char buf[Size];

public:
    CellPtr operator & ()       { return &len; }
    Char &operator [] (int n)   { return buf[n]; }
    Cell &length()              { return len; }
};



# define    FLD_OFS(t,f)        (((CharPtr) &(((t *) 0)->f)) - ((CharPtr) ((t *) 0)))


# define    GOTO_ADR(L)         (Cell(&&L) - Cell(&&$$HEAD))
//define    GOTO_ADR(L)         (Cell(&&L))


void engine_init(bool no_ok);
void *engine_thread(void *startWord);
Cell engine();


struct MemRegion {      // record all interesting memory areas for forensic purposes
    typedef MemRegion *Ptr;

    const char *cpName;
    const char *cpStart;
    const char *cpEnd;
    CellPtr handler;
};


struct SpecialWordNames {
    Cell clWordAddr;
    Cell length;
    char name[32];
};


struct Environment {
    Cell length;
    char name[32];
    Cell value;
    Cell handler;
};

