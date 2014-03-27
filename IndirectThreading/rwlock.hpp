
enum rwl_prefer_e { RWL_PREF_READ, RWL_PREF_WRITE };
enum rwl_lock_e { RWL_NOTHING, RWL_READ, RWL_WRITE };      // so callers can record their own lock choice

class rwlock_t {                // Structure describing a read/write lock.
    pthread_mutex_t mutex;
    pthread_cond_t read;        // wait for read
    pthread_cond_t write;       // wait for write
    int valid;                  // set when valid
    short int r_wait;           // readers waiting
    short int w_wait;           // writers waiting
    short int r_active;         // readers active
    bool w_active;              // writer active
    bool prefer_write;          // true if writers to be given precedence

    static const int RWLOCK_VALID = 0xfacade;

    static void readCleanup(void *arg);
    static void writeCleanup(void *arg);

public:

    rwlock_t(rwl_prefer_e pw = RWL_PREF_READ) :
        r_wait(0), r_active(0),
        w_wait(0), w_active(false),
        valid(0),
        prefer_write(pw != RWL_PREF_READ)
    { }

    ~rwlock_t() { }

    int init();
    int destroy();
    int readLock();
    int readTryLock();
    int readUnlock();
    int writeLock();
    int writeTryLock();
    int writeUnlock();
};

