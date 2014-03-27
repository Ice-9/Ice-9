
# ifndef __WORDSIZE
#  define __WORDSIZE 32
# endif

typedef void                 *VoidPtr;
typedef char        Char,    *CharPtr;
typedef uint8_t     Byte,    *BytePtr;
typedef uint32_t    UniChar, *UniCharPtr;             // 21 bits needed for Unicode
// this definition of UniChar conflicts with the choice to store Unicode in 3 bytes

typedef intptr_t    Int,     *IntPtr;       // int the size of a native pointer
typedef int16_t     Int16,   *Int16Ptr;
typedef int32_t     Int32,   *Int32Ptr;
typedef int64_t     Int64,   *Int64Ptr;

typedef uintptr_t   Word,    *WordPtr;      // uint the size of a native pointer
typedef uint16_t    Word16,  *Word16Ptr;
typedef uint32_t    Word32,  *Word32Ptr;
typedef uint64_t    Word64,  *Word64Ptr;

// TODO: note how the float types are machine dependent - lock it down!
// Also, exact size of FloatLD is vague.
typedef float       Float32, *Float32Ptr;
typedef double      Float64, *Float64Ptr;
typedef long double FloatLD, *FloatLDPtr;

# define    MAX_FLOAT32         FLT32_MAX
# define    MAX_FLOAT64         FLT64_MAX
# define    MAX_FLOATLD         LDBL_MAX

// probably
# if __WORDSIZE == 16
typedef Word16 Cell;
typedef Word32 Cell2;
typedef Int16 sCell;
typedef Int32 sCell2;

#  define   HIGH_BIT            0x8000
#  define   MAX_CELL            UINT16_MAX
#  define   MAX_SCELL           INT16_MAX
#  define   MAX_2CELL           UINT32_MAX
#  define   MAX_2SCELL          INT32_MAX
# endif

// 64 bit builds can get 64 bit cells when the double-cell concept from FORTH is abandoned
// unless the ability to do 128 bit math is discovered in gcc
# if __WORDSIZE == 32 || __WORDSIZE == 64
typedef Word32 Cell;
typedef Word64 Cell2;
typedef Int32 sCell;
typedef Int64 sCell2;

#  define   HIGH_BIT            0x80000000
#  define   MAX_CELL            UINT32_MAX
#  define   MAX_SCELL           INT32_MAX
#  define   MAX_2CELL           UINT64_MAX
#  define   MAX_2SCELL          INT64_MAX
# endif

typedef Cell  *CellPtr;   typedef Cell2  *Cell2Ptr;
typedef sCell *sCellPtr;  typedef sCell2 *sCell2Ptr;

typedef CellPtr *CellPtrPtr;

