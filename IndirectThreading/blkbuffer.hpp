
class BlockBuffer {
public:
    typedef BlockBuffer *Ptr;

private:
    BlockBuffer::Ptr bbpNext, bbpPrev;

    CharPtr buf;
    sCell blkNo;
    bool updated;

public:
    static THREAD_VAR BlockBuffer bufs[NUM_BLK_BUFFERS];
    static THREAD_VAR BlockBuffer::Ptr bbpLRU, bbpMRU;
    static THREAD_VAR BlockBuffer::Ptr bbpCurrent;
    static THREAD_VAR sCell fd;

private:
    void clear();
    void destroy();
    bool write();
    bool read();
    bool flush();
    static BlockBuffer::Ptr findBlkNo(sCell bn);
    static BlockBuffer::Ptr allocBuffer();
    static void LRUpromote(BlockBuffer::Ptr bbp);
    static BlockBuffer::Ptr BlockWork(Cell bn, bool loadem);
    static void SaveAll(bool purge);
    static void relinkLRU();
    static void EmptyAll();

public:
    static void initQueue(const char *fname);
    static void destroyQueue();
    static CharPtr Block(Cell bn);      // perform duty of BLOCK - load from disk if not found
    static CharPtr Buffer(Cell bn);     // perform duty of BUFFER
    static void flush_buffers();        // perform duty of FLUSH
    static void save_buffers();         // perform duty of SAVE-BUFFERS
    static void empty_buffers();        // perform duty of EMPTY-BUFFERS
    static void update_current();
};

