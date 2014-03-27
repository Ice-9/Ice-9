
# ifndef _NODE_H_
#  define _NODE_H_


namespace Ice9 {
    namespace Memory {
        // a NodePtr can store: true pointer, encoded integer (short by 2 bits), qualifier pointer, or reference to a dimensioned object
        // The pointer form is a true address, so the low 2 bits will be zero (32bit CPU and above to hell with anything else for this API)
        // This implies all allocations are aligned to at least 4 bytes
        // An Integer is shifted by 2 and has 01 in the low bits. Total bits used for the Integer is (NativeBits - 2)
        // A Qualifier is a true hardware pointer but spoiled with 10 in the 2 lowest bits
        // A Dimensioned object pointer is a true hardware pointer but spoiled with 11 in the 2 lowest bits.


        struct NodeHdr {
            friend int main(int argc, char *argv[]);
            friend class OP;

            Word bszNode;               // ALWAYS in bytes; ALWAYS includes the size of the Header

            union {
                Word _w;               // to help lock-in the alignment of the next thingy (Explorers note: back when I was using types other than Word in it?)
                struct {
                    Word
                        RC : 8,         // ref count when live
        # define    RC_STUCK    UINT8_MAX
        # define    HUGE_SIZE   UINT8_MAX

                        // TODO: tailor the number of bits to suit the wordsize
                        //
                        // depending on the alignment needs of the Node structure, forced by the alignment needs
                        // of the worst-case native type, there can be extra bytes in the allocation that
                        // needs to be hidden from the API. Upto 32 bytes of extra can be allowed for.
                        // Note that the extra is counted in bytes always, since msize above can be anything.
                        //
                        // NOTE also: since each object requires at least one word for the npNext freelist pointer,
                        // any attempt to allocate an object smaller than WORD_SIZE (user space size) will be
                        // padded with extra bytes to become WORD_SIZE.

                        spare : 2,
                        oversize : 1,   // for pointer (but not qualifier) arrays which need an extra word for GC if GC can't temporarily store the size in RC
                        isptrs : 1,     // non-zero if the object contains OPs - affects GC. msize is irrelevant and should be zero
                                        // IF zero, msize becomes relevant

                        // msize = member size - 1, where a member is the arrayed things stored within
                        // When isptrs is zero, msize encodes the bytesize of the objects.
                        // Note: wierd-ass sizes are not expected but this is easier than silly encoding ideas...
                        // TODO: if msize is available reliably, then maybe all the <Type>At() methods can use it instead of the constants
                        //
                        msize : 4,      // allows upto 128-bit scalar member (16 bytes)
                                        // TODO: decide if it should be set nominally to correct non-zero value for pointers or qualifiers
                                        // can't see the point, since it's irrelevant if isptrs==0!

                        extra : 5,      // 0..ALIGN_SIZE_BYTES-1 bytes extra covering overallocation

                        // mofs = member offset
                        // for types requiring hardware alignment (eg floating points) this encodes
                        // how many bytes are ignored from the start of byChild to get to the real item.
                        // NOTE: not an offset from the beginning of the block! just within byChild[]
                        // Always includes the bytes pre-allocated for the class pointer, so even an
                        // item with byte alignment of 1 will see a non-zero value in mofs.
                        // Also, only big enough for a native requiring upto 32 bytes alignment.
                        // Can't think of any ...
                        //
                        mofs : 5,

                        // number of dimensions - 0 for scalar. Limit is 63 which is insane
                        dims : 6;
                        // TODO: bugger - left-over crap in dims will blow up x - 'x'? What is 'x'?     WAAAAAAAAA? Is he talking about DEFERRED DOWN getting hit by a leak? Or something?
                        // explorers note: until the object is deferred-downed, dims needs to remain in place
                        // UNLESS: can the object  be converted to a scalar like a traditional object?
                        // OR: at least convert it to a vector, add a bit to warn GC, and then this thing can be set to zero
                        // but first: find out what the hell the imagined problem was/is...
                };
            };
        };  // NodeHdr::


        // change to class when no longer need visibilities for debugging - OH! I DID ALREADY!
        class Node {                   // strictly POD! no virtuals etc
            friend class OP;
            friend class Allocator;
            friend class Extent;


            // SPACE allocation
            // every Allocation whether in use or on the free list has a bytesize in the first word
            // and bit-flags in the 2nd word. Therefore it's membership on any freelist is stored
            // in the first word of the object space. See the npNext member of the union below.
            NodeHdr hdr;

            # define    nh_bszNode      hdr.bszNode
            # define    nh__w           hdr._w
            # define    nh_RC           hdr.RC
            # define    nh_extra        hdr.extra
            # define    nh_isptrs       hdr.isptrs
            # define    nh_oversize     hdr.oversize
            # define    nh_msize        hdr.msize
            # define    nh_mofs         hdr.mofs
            # define    nh_dims         hdr.dims


            // this won't necessarily be aligned to suit all objects,
            // but other code will pretend it starts at a suitable point for it's amusement
            //
            union {
                NodePtr npNext;         // when on the free list
                OP      opChilds[0];    // MI objects can have several class words
                Byte    byChilds[0];
                Word    wChilds[0];
            };

            // TODO: confirm that CheckAccess is coping properly with the existence of the dimensions
            // NO IT ISN'T! CheckAccess, accessNP and updateNP are ONLY usable for scalar class objects
            // since all dimensioned objects must be references only, then CheckAccess, accessNP and updateNP
            // must not be used
            //
            void CheckAccess(Word, bool);

            inline void CountDown() { if(nh_RC < RC_STUCK) --nh_RC; }
            inline void CountUp  () { if(nh_RC < RC_STUCK) ++nh_RC; }

            inline Word StartWord();   inline Word StartPtr();
            inline Word LimitWord();   inline Word LimitPtr();

            // usable byte/word size from the user's perspective
            // - excludes the header, the extra word and the member padding offset if present
            inline Word bszData();
            inline Word wszData();

        public:
            static const NodePtr NIL;

        };  /* Node:: */

        static const int NODE_SIZE = sizeof(Node);
        static const int NHDR_SIZE = sizeof(NodeHdr);    // TODO:shouldn't this be available to the .hpp and it's users???


    };  /* namespace Ice9::Memory */
};  /* namespace Ice9 */

# endif

