
# include       "tilengine.hpp"
# include       "blkbuffer.hpp"

void BlockBuffer::clear() {
    buf = 0;
    blkNo = -1;
    updated = false;
    // bbpNext = NULL;
    // bbpPrev = NULL;
}


void BlockBuffer::destroy() {
    if(buf)
        delete [] buf;
    clear();
}


bool BlockBuffer::write() {
    // silently ignore a failed block opener
    // - initQueue() needs to spew or something
    //
    if(fd < 0) {
        errno = EBADF;
        perror("BlockBuffer::write");
        return false;
    }

    if(::lseek(fd, (blkNo - 1) * SIZE_BLK_BUFFER, 0) == (off_t) -1) {
        perror("lseek() within BlockBuffer::write()");
        return false;
    }

    if(::write(fd, buf, SIZE_BLK_BUFFER) == -1) {
        perror("write() within BlockBuffer::write()");
        return false;
    }

    updated = 0;
    return true;
}


bool BlockBuffer::read() {
    // silently ignore a failed block opener
    // - initQueue() needs to spew or something
    //
    if(fd < 0) {
        errno = EBADF;
        perror("BlockBuffer::read");
        return false;
    }

    if(::lseek(fd, (blkNo - 1) * SIZE_BLK_BUFFER, 0) == (off_t) -1) {
        perror("lseek() within BlockBuffer::read()");
        return false;
    }

    if(::read(fd, buf, SIZE_BLK_BUFFER) == -1) {
        perror("read() within BlockBuffer::write()");
        return false;
    }

    return true;
}


bool BlockBuffer::flush() {
    if(updated)
        return write();
    return true;
}


BlockBuffer::Ptr BlockBuffer::findBlkNo(sCell bn) {
    for(int i = 0; i < NUM_BLK_BUFFERS; ++i)
        if(bufs[i].blkNo == bn)
            return &bufs[i];
    return NULL;
}


BlockBuffer::Ptr BlockBuffer::allocBuffer() {
    if(bbpLRU->buf == NULL)
        bbpLRU->buf = CharPtr(malloc(SIZE_BLK_BUFFER));
    else
        (void) bbpLRU->flush();

    return bbpLRU;
}


void BlockBuffer::LRUpromote(BlockBuffer::Ptr bbp) {
    // break it out of the linked lists
    if(! bbp->bbpNext)      // is it already MRU?
        return;             // if so, nothing to do

    // skip around this one
    bbp->bbpNext->bbpPrev = bbp->bbpPrev;
    if(bbp->bbpPrev)
        bbp->bbpPrev->bbpNext = bbp->bbpNext;
    else
        bbpLRU = bbp->bbpNext;

    // relink this one to the MRU end
    bbp->bbpNext = NULL;
    bbp->bbpPrev = bbpMRU;
    bbp->bbpPrev->bbpNext = bbp;
    bbpMRU = bbp;
}


BlockBuffer::Ptr BlockBuffer::BlockWork(Cell bn, bool loadem) {
    BlockBuffer::Ptr bbp = findBlkNo(bn);
    if(bbp == NULL && (bbp = allocBuffer())) {
        bbp->blkNo = bn;

        if(loadem && ! bbp->read()) {   // failure to read?
            bbp->blkNo = 0;
            bbp = NULL;
        }
    }

    bbpCurrent = bbp;
    if(bbp)
        LRUpromote(bbp);
    return bbp;
}


void BlockBuffer::SaveAll(bool purge) {
    for(int i = 0; i < NUM_BLK_BUFFERS; ++i) {
        BlockBuffer::Ptr bbp = &bufs[i];

        if(! bbp->buf)
            continue;

        bbp->flush();

        if(purge)
            bbp->destroy();
    }

    if(purge)
        bbpCurrent = NULL;

    if(fd >= 0)
        fsync(fd);
}


void BlockBuffer::relinkLRU() {
    for(int i = 0; i < NUM_BLK_BUFFERS; ++i) {
        bufs[i].bbpNext = &bufs[i + 1];
        bufs[i].bbpPrev = &bufs[i - 1];
    }

    bbpLRU = &bufs[0];
    bbpMRU = &bufs[NUM_BLK_BUFFERS - 1];
    bbpLRU->bbpPrev = NULL;
    bbpMRU->bbpNext = NULL;
}


void BlockBuffer::EmptyAll() {
    for(int i = 0; i < NUM_BLK_BUFFERS; ++i)
        bufs[i].destroy();
    relinkLRU();
}


void BlockBuffer::initQueue(const char *fname) {
    bbpCurrent = NULL;

    // TODO: delay the open until some activity starts to happen?
    //
    fd = open(fname, O_RDWR | O_CREAT, 0666);
    if(fd == -1)
        perror("block buffer initQueue cannot open or create BLOCK file");

    for(int i = 0; i < NUM_BLK_BUFFERS; ++i)
        bufs[i].clear();
    relinkLRU();
}


void BlockBuffer::destroyQueue() {
    SaveAll(true);

    if(fd >= 0) {
        close(fd);
        fd = -1;
    }

    bbpCurrent = NULL;
}


CharPtr BlockBuffer::Block(Cell bn) {    // perform duty of BLOCK - load from disk if not found
    return BlockWork(bn, true)->buf;
}


CharPtr BlockBuffer::Buffer(Cell bn) {   // perform duty of BUFFER
    return BlockWork(bn, false)->buf;
}


void BlockBuffer::flush_buffers() {   // perform duty of FLUSH
    SaveAll(true);
}


void BlockBuffer::save_buffers() {    // perform duty of SAVE-BUFFERS
    SaveAll(false);
}


void BlockBuffer::empty_buffers() {   // perform duty of EMPTY-BUFFERS
    EmptyAll();
}


void BlockBuffer::update_current() {
    if(bbpCurrent)
        bbpCurrent->updated = true;
}

