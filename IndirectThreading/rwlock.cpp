
# include       "tilengine.hpp"


int rwlock_t::init() {      // Initialize a read/write lock.
    int status;

    status = pthread_mutex_init(&mutex, NULL);
    if(status)
        return status;

    status = pthread_cond_init(&read, NULL);
    if(status) {
        // if unable to create read CV, destroy mutex
        pthread_mutex_destroy(&mutex);
        return status;
    }

    status = pthread_cond_init(&write, NULL);
    if(status != 0) {
        // if unable to create write CV, destroy read CV and mutex
        pthread_cond_destroy(&read);
        pthread_mutex_destroy(&mutex);
        return status;
    }

    valid = RWLOCK_VALID;
}   /* rwlock_t::init */


int rwlock_t::destroy() {    // Destroy a read/write lock.
    int status, status1, status2;

    if(valid != RWLOCK_VALID)
        return EINVAL;

    status = pthread_mutex_lock(&mutex);
    if(status != 0)
        return status;

    // Check whether any threads own the lock; report "BUSY" if so.
    if(r_active > 0 || w_active) {
        pthread_mutex_unlock(&mutex);
        return EBUSY;
    }

    // Check whether any threads are known to be waiting; report EBUSY if so.
    if(r_wait > 0 || w_wait > 0) {
        pthread_mutex_unlock(&mutex);
        return EBUSY;
    }

    valid = 0;
    status = pthread_mutex_unlock(&mutex);
    if(status != 0)
        return status;

    status = pthread_mutex_destroy(&mutex);
    status1 = pthread_cond_destroy(&read);
    status2 = pthread_cond_destroy(&write);

    return status  != 0 ? status  :
           status1 != 0 ? status1 :
           status2;

}   /* rwlock_t::destroy */


// Handle cleanup when the read lock condition variable wait is canceled.
// Simply record that the thread is no longer waiting, and unlock the mutex.

void rwlock_t::readCleanup(void *arg) {
    rwlock_t *rwl = (rwlock_t *) arg;

    rwl->r_wait--;
    pthread_mutex_unlock(&rwl->mutex);
}   /* rwlock_t::readCleanup */


// Handle cleanup when the write lock condition variable wait is canceled.
// Simply record that the thread is no longer waiting, and unlock the mutex.

void rwlock_t::writeCleanup(void *arg) {
    rwlock_t *rwl = (rwlock_t *) arg;

    rwl->w_wait--;
    pthread_mutex_unlock(&rwl->mutex);
}   /* rwlock_t::writeCleanup */



// Lock a read/write lock for read access.
int rwlock_t::readLock() {
    int status;

    if(valid != RWLOCK_VALID)
        return EINVAL;

    status = pthread_mutex_lock(&mutex);
    if(status != 0)
        return status;

    if(w_active || (prefer_write && w_wait)) {
        r_wait++;
        pthread_cleanup_push(rwlock_t::readCleanup, (void *) this);

        while(w_active || (prefer_write && w_wait)) {
            status = pthread_cond_wait(&read, &mutex);
            if(status != 0)
                break;
        }

        pthread_cleanup_pop(0);
        r_wait--;
    }

    if(status == 0)
        r_active++;

    pthread_mutex_unlock(&mutex);
    return status;
}   /* rwlock_t::readLock */



// Attempt to lock a read/write lock for read access (don't block if unavailable).
int rwlock_t::readTryLock() {
    int status, status2;

    if(valid != RWLOCK_VALID)
        return EINVAL;

    status = pthread_mutex_lock(&mutex);
    if(status != 0)
        return status;

    if(w_active || (prefer_write && w_wait))
        status = EBUSY;
    else
        r_active++;

    status2 = pthread_mutex_unlock(&mutex);
    return status2 != 0 ? status2 : status;
}   /* rwlock_t::readTryLock */



// Unlock a read/write lock from read access.
int rwlock_t::readUnlock() {
    int status, status2;

    if(valid != RWLOCK_VALID)
        return EINVAL;

    status = pthread_mutex_lock(&mutex);
    if(status != 0)
        return status;

    r_active--;
    if(r_active == 0 && w_wait > 0)
        status = pthread_cond_signal(&write);

    status2 = pthread_mutex_unlock(&mutex);
    return status2 == 0 ? status : status2;
}   /* rwlock_t::readUnlock */



// Lock a read/write lock for write access.
int rwlock_t::writeLock() {
    int status;

    if(valid != RWLOCK_VALID)
        return EINVAL;

    status = pthread_mutex_lock(&mutex);
    if(status != 0)
        return status;

    if(w_active || r_active > 0) {
        w_wait++;
        pthread_cleanup_push(rwlock_t::writeCleanup, (void *) this);

        while(w_active || r_active > 0) {
            status = pthread_cond_wait(&write, &mutex);
            if(status != 0)
                break;
        }

        pthread_cleanup_pop(0);
        w_wait--;
    }

    if(status == 0)
        w_active = 1;

    pthread_mutex_unlock(&mutex);
    return status;
}   /* rwlock_t::writeLock */



// Attempt to lock a read/write lock for write access. Don't block if unavailable.
int rwlock_t::writeTryLock() {
    int status, status2;

    if(valid != RWLOCK_VALID)
        return EINVAL;

    status = pthread_mutex_lock(&mutex);
    if(status != 0)
        return status;

    if(w_active || r_active > 0)
        status = EBUSY;
    else
        w_active = 1;

    status2 = pthread_mutex_unlock(&mutex);
    return status != 0 ? status : status2;
}   /* rwlock_t::writeTryLock */



// Unlock a read/write lock from write access.
int rwlock_t::writeUnlock() {
    int status;

    if(valid != RWLOCK_VALID)
        return EINVAL;

    status = pthread_mutex_lock(&mutex);
    if(status != 0)
        return status;

    w_active = 0;

    if(r_wait > 0 && (! prefer_write || w_wait == 0)) {
        status = pthread_cond_broadcast(&read);
        if(status != 0) {
            pthread_mutex_unlock(&mutex);
            return status;
        }
    }
    else if(w_wait > 0) {
        status = pthread_cond_signal(&write);
        if(status != 0) {
            pthread_mutex_unlock(&mutex);
            return status;
        }
    }

    status = pthread_mutex_unlock(&mutex);
    return status;
}    /* rwlock_t::writeUnlock */

