#ifndef __CAPE_SYS__QUEUE__H
#define __CAPE_SYS__QUEUE__H 1

#include "sys/cape_export.h"
#include "sys/cape_err.h"

//-----------------------------------------------------------------------------

struct CapeQueue_s; typedef struct CapeQueue_s* CapeQueue;

typedef void (__STDCALL *cape_queue_cb_fct)(void* ptr);

//-----------------------------------------------------------------------------

__CAPE_LIBEX   CapeQueue   cape_queue_new          (void);

                           /*
                            * frees memory and wait for all threads to terminate
                            * -> if there is a task running, it blocks until it has been finished
                            */
__CAPE_LIBEX   void        cape_queue_del          (CapeQueue*);

//-----------------------------------------------------------------------------

                           /*
                            * waits for the next event (blocking)
                            */
__CAPE_LIBEX   int         cape_queue_next         (CapeQueue);

                           /*
                            * adds a new task
                            */
__CAPE_LIBEX   void        cape_queue_add          (CapeQueue, cape_queue_cb_fct on_event, cape_queue_cb_fct on_done, void* ptr);

                           /*
                            * starts the queueing in background
                            * -> threads will be created
                            * -> amount of threads defines the worker threads waiting for events
                            */
__CAPE_LIBEX   int         cape_queue_start        (CapeQueue, int amount_of_threads, CapeErr err);

                           /*
                            * waits until the queue is empty
                            */
__CAPE_LIBEX   void        cape_queue_wait         (CapeQueue);

//-----------------------------------------------------------------------------

#endif
