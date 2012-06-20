#pragma once

#include <cassert>
#include <pthread.h>

class scoped_lock {
 public:
    scoped_lock(pthread_mutex_t *muarg) : mu(muarg) {
        int r = pthread_mutex_lock(mu);
        if (r) assert(false);
    }

    ~scoped_lock() {
        int r = pthread_mutex_unlock(mu);
        if (r) assert(false);
    }

 private:
    pthread_mutex_t *mu;
};
