/*
 * NAME, etc.
 *
 * sync.c
 *
 * Synchronization routines for SThread
 */

#define _REENTRANT

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <sched.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include "sthread.h"

/*
 * Atomic operations for x86 architecture.
 */
static inline int test_and_set_bit(volatile unsigned long *addr)                /*treats 0 as availible, 1 as protected*/
{
	int oldval;
	__asm__ __volatile__("xchgl %0, %1"
			: "=r"(oldval), "+m"(*(addr))	/* output */
			: "0"(1)						/* input */
			: "memory"	/* clobbered: changing contents of memory */
			);
	return oldval;
}
static inline void clear_bit(volatile unsigned long *addr)
{
	unsigned long oldval;
	__asm__ __volatile__("xchgl %0, %1"
			: "=r"(oldval), "+m"(*(addr))	/* output */
			: "0"(0)						/* input */
			: "memory"	/* clobbered: changing contents of memory */
			);
}

/*
 * rwlock routines
 */

int sthread_rwlock_init(sthread_rwlock_t *rwlock)
{
        rwlock->protect = 0;   /*protection lock is free at 0*/
        rwlock->state = 0;      /*state, what the RW doing...0=avalible 1=read lock is held 2=write lock is held*/
        rwlock->head= NULL;     /*head will be empty because queue is empty*/
        return 0;
}

int sthread_rwlock_destroy(sthread_rwlock_t *rwlock)    /*free resources associated with RW lock*/
{
        struct waiting_threads *current;        /*keep track of current waiting thread*/
        struct waiting_threads *next;           /*keeps track of next waiting thread*/
        current = rwlock->head;                 //start at the first waiting thread
        while(current != NULL)          //continue while there are waiting threads
        {
                next =  current ->next;         //save thread
                free(current);                  //free current thread
                current = next;                 //move to next thread in waiting
        }

        rwlock->head= NULL;  //waiting list is empty
        return 0;   
}

int sthread_read_lock(sthread_rwlock_t *rwlock)
{
        struct waiting_threads *new_thread;
        while(1){

        
        while (test_and_set_bit(&rwlock->protect)) {       //take the protect spinlock
                sched_yield();          //give another thread a chance to run
        }    
        if (rwlock->state==0)
        {
                rwlock->state = 1;              //mark rwlock as being used by a reader
                clear_bit(&rwlock->protect);    //release protection spinlock
                return 0;                       //readlock succesful
        }                                       //now i need to implement the condition for when the state is not 0
        new_thread = malloc(sizeof(struct waiting_threads));            //create space for waiting thread
        if(new_thread==NULL)
        {
                clear_bit(&rwlock->protect);
                return -1;
        }
        new_thread->thread = sthread_self();                    //store current thread in waiting node
        new_thread->next=rwlock->head;
        rwlock->head=new_thread;                //add the waiting thread to the front of the list
        clear_bit(&rwlock->protect);            //release the protection spinlock
        sthread_suspend();                       //suspend until woken up by another thread
        }
        return -1;                               /*this should be unreachable but gcc doesnt like when i compile without it*/

}   
int sthread_read_try_lock(sthread_rwlock_t *rwlock)
{
        while(test_and_set_bit(&rwlock->protect))
        {
                sched_yield();                          //give another thread a chance to run
        }
                                                        //if the rwlock is free
        if(rwlock->state==0)
        {
                rwlock->state = 1;                      //mark rwlock as a reader
                clear_bit(&rwlock->protect); 
        
        return 0;                                       //readlock was sucessfully obtained
}
        clear_bit(&rwlock->protect);
        return 1; //rw was not available
}
int sthread_read_unlock(sthread_rwlock_t *rwlock)
{
        struct waiting_threads *waiting;
        while(test_and_set_bit(&rwlock->protect))
        {
                sched_yield();
        }
         if(rwlock->state!=1)
        {
                clear_bit(&rwlock->protect);
                return -1; //unlock
        }
        rwlock->state=0;                       //mark lock as available
        if(rwlock->head != NULL){
                waiting = rwlock->head;  //get the first waiting thread
                rwlock->head=waiting->next; //move head to next waiting thread
                sthread_wake(waiting->thread); //wake waiting thread
                free(waiting);
        }
        clear_bit(&rwlock->protect);
        return 0;
}

int sthread_write_lock(sthread_rwlock_t *rwlock)
{
        struct waiting_threads *new_thread;
        while(1)
        {
                while(test_and_set_bit(&rwlock->protect))
                {
                        sched_yield();
                }
        if(rwlock->state==0)
        {
                rwlock->state=2; //2 state means they are marked as a writer
                clear_bit(&rwlock->protect);
                return 0; //succesfully acquired write lock
        }
        new_thread = malloc(sizeof(struct waiting_threads)); //memory space for waiting threads
        if (new_thread==NULL)
        {
                clear_bit(&rwlock->protect);
                return -1;
        }
        new_thread->thread = sthread_self();   //store node in waiting node
        new_thread->next= rwlock->head;
        rwlock->head = new_thread;
        clear_bit(&rwlock->protect);
        sthread_suspend();
        }
        return -1;
}

int sthread_write_try_lock(sthread_rwlock_t *rwlock)
{
        while(test_and_set_bit(&rwlock->protect))
        {
        sched_yield();
        }
        if(rwlock->state==0)
        {
                rwlock->state = 2;              //mark as used by writer
                clear_bit(&rwlock->protect);    //release spin protection
                return 0;
        }
        clear_bit(&rwlock->protect);
        return 1;
}

int sthread_write_unlock(sthread_rwlock_t *rwlock)
{
    struct waiting_threads *waiting;
    while(test_and_set_bit(&rwlock->protect))
        {
                sched_yield();
        }
      
        if(rwlock->state !=2){
                clear_bit(&rwlock->protect);
                return -1;              //unlock failed
        }
        rwlock->state=0; //mark the RW lock as avalible

        if(rwlock->head != NULL)
        {
                waiting = rwlock->head; //get first thread
                rwlock->head= waiting->next;    //move head to next waiting
                sthread_wake(waiting->thread); //wake waiting node      
                free(waiting);
        }
        clear_bit(&rwlock->protect);
        return 0;
}
