/*
 * NAME, etc.
 *
 * sync.h
 */

#ifndef _STHREAD_SYNC_H_
#define _STHREAD_SYNC_H_


struct waiting_threads {                 /*  position for waiting threads    */
        sthread_t thread;               /*stores what thread is waiting in thread, acts as a pointer to sthread_struct*/
        struct waiting_threads *next;    /*points to another waiting thread, if there is one*/
};

struct sthread_rwlock_struct {                 /*volatile so that the value is checked whenever it is accessed*/
        volatile unsigned long protect;
        int state;                              /*keeps track of the state of the RW lock, 0=free 1=read locked 2=write locked*/
        struct waiting_threads *head;                   /*points to the first waiting thread*/
};

typedef struct sthread_rwlock_struct sthread_rwlock_t;

int sthread_rwlock_init(sthread_rwlock_t *rwlock);
int sthread_rwlock_destroy(sthread_rwlock_t *rwlock);
int sthread_read_lock(sthread_rwlock_t *rwlock);
int sthread_read_try_lock(sthread_rwlock_t *rwlock);
int sthread_read_unlock(sthread_rwlock_t *rwlock);
int sthread_write_lock(sthread_rwlock_t *rwlock);
int sthread_write_try_lock(sthread_rwlock_t *rwlock);
int sthread_write_unlock(sthread_rwlock_t *rwlock);

#endif
