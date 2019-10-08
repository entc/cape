#include "cape_queue.h"

//-----------------------------------------------------------------------------

// cape includes
#include "sys/cape_types.h"
#include "sys/cape_log.h"
#include "sys/cape_mutex.h"
#include "sys/cape_thread.h"
#include "stc/cape_list.h"

#if defined __WINDOWS_OS

#else

// linux includes
#include <unistd.h>
#include <semaphore.h>
#include <sys/ipc.h>
#include <sys/sem.h>

#endif

//-----------------------------------------------------------------------------

struct CapeSync_s
{
  int semid;
};

//-----------------------------------------------------------------------------

CapeSync cape_sync_new (void)
{
  CapeSync self = CAPE_NEW (struct CapeSync_s);
  
  self->semid = semget (IPC_PRIVATE, 1, IPC_CREAT  | IPC_EXCL | 0666);
  
  if (self->semid == -1)
  {
    CapeErr err = cape_err_new ();
    
    cape_err_lastOSError (err);
    
    cape_log_fmt (CAPE_LL_ERROR, "CAPE", "sync del", "can't permforme semaphore action: %s", cape_err_text(err));
    
    cape_err_del (&err);
  }
  
  semctl (self->semid, 0, SETVAL, 0);
  
  return self;
}

//-----------------------------------------------------------------------------

void cape_sync_del (CapeSync* p_self)
{
  if (*p_self)
  {
    CapeSync self = *p_self;

    cape_sync_wait (self);
    
    close (self->semid);

    CAPE_DEL (p_self, struct CapeSync_s);
  }
}

//-----------------------------------------------------------------------------

void cape_sync_inc (CapeSync self)
{
  if (self)
  {
    struct sembuf sops[1];

    sops[0].sem_num = 0;
    sops[0].sem_op = 1;
    sops[0].sem_flg = 0;

    int res = semop (self->semid, sops, 1);
    
    if (res == -1)
    {
      CapeErr err = cape_err_new ();
      
      cape_err_lastOSError (err);
      
      cape_log_fmt (CAPE_LL_ERROR, "CAPE", "sync del", "can't permforme semaphore action: %s", cape_err_text(err));
      
      cape_err_del (&err);
    }
  }
}

//-----------------------------------------------------------------------------

void cape_sync_dec (CapeSync self)
{
  if (self)
  {
    struct sembuf sops[1];
    
    sops[0].sem_num = 0;
    sops[0].sem_op = -1;
    sops[0].sem_flg = 0;
    
    int res = semop (self->semid, sops, 1);    

    if (res == -1)
    {
      CapeErr err = cape_err_new ();
      
      cape_err_lastOSError (err);
      
      cape_log_fmt (CAPE_LL_ERROR, "CAPE", "sync del", "can't permforme semaphore action: %s", cape_err_text(err));
      
      cape_err_del (&err);
    }
  }
}

//-----------------------------------------------------------------------------

void cape_sync_wait (CapeSync self)
{
  if (self)
  {
    struct sembuf sops[1];
    
    sops[0].sem_op = 0;
    sops[0].sem_flg = 0;
    sops[0].sem_num = 0;
    
    int res = semop (self->semid, sops, 1);
    
    if (res == -1)
    {
      CapeErr err = cape_err_new ();
      
      cape_err_lastOSError (err);
      
      cape_log_fmt (CAPE_LL_ERROR, "CAPE", "sync del", "can't permforme semaphore action: %s", cape_err_text(err));
      
      cape_err_del (&err);
    }
  }
}

//-----------------------------------------------------------------------------

struct CapeQueue_s
{
  CapeMutex mutex;
  
  CapeList threads;
  
  CapeList queue;
  
  sem_t sem;    // semaphore structure
  
  int terminated;
};

//-----------------------------------------------------------------------------

struct CapeThreadItem_s
{
  CapeThread thread;
  
  CapeQueue queue;
  
}; typedef struct CapeThreadItem_s* CapeThreadItem;

//-----------------------------------------------------------------------------

void __STDCALL cape_queue__threads__on_del (void* ptr)
{
  CapeThreadItem self = ptr;
  
  cape_thread_join (self->thread);
  
  cape_thread_del (&(self->thread));
}

//-----------------------------------------------------------------------------

struct CapeQueueItem_s
{
  cape_queue_cb_fct on_event;
  
  cape_queue_cb_fct on_done;
  
  void* ptr;
  
  CapeSync sync;    // reference
  
}; typedef struct CapeQueueItem_s* CapeQueueItem;

//-----------------------------------------------------------------------------

void __STDCALL cape_queue__item__on_del (void* ptr)
{
  CapeQueueItem item = ptr;
  
  if (item->on_done)
  {
    item->on_done (item->ptr);
  }
  
  cape_sync_dec (item->sync);
  
  CAPE_DEL (&item, struct CapeQueueItem_s);
}

//-----------------------------------------------------------------------------

CapeQueue cape_queue_new (void)
{
  CapeQueue self = CAPE_NEW (struct CapeQueue_s);
  
  self->mutex = cape_mutex_new ();

  sem_init (&(self->sem), 0, 0);
  
  self->terminated = FALSE;
  
  self->threads = cape_list_new (cape_queue__threads__on_del);
  
  self->queue = cape_list_new (cape_queue__item__on_del);
  
  return self; 
}

//-----------------------------------------------------------------------------

void cape_queue_del (CapeQueue* p_self)
{
  if (*p_self)
  {
    CapeQueue self = *p_self;
    
    self->terminated = TRUE;
    
    {
      CapeListCursor* cursor = cape_list_cursor_create (self->threads, CAPE_DIRECTION_FORW);
      
      while (cape_list_cursor_next (cursor))
      {
        // activate the thread
        sem_post (&(self->sem));
      }
      
      cape_list_cursor_destroy (&cursor);
    }
    
    cape_list_del (&(self->threads));

    cape_list_del (&(self->queue));
    
    cape_mutex_del (&(self->mutex));
    
    CAPE_DEL (p_self, struct CapeQueue_s);
  }
}

//-----------------------------------------------------------------------------

static int __STDCALL cape_queue__worker__thread (void* ptr)
{
  while (cape_queue_next (ptr));
  
  cape_log_msg (CAPE_LL_TRACE, "CAPE", "queue start", "thread terminated");
  
  return 0;
}

//-----------------------------------------------------------------------------

int cape_queue_start  (CapeQueue self, int amount_of_threads, CapeErr err)
{
  int i;
  
  for (i = 0; i < amount_of_threads; i++)
  {
    CapeThreadItem ti = CAPE_NEW (struct CapeThreadItem_s);
    
    ti->thread = cape_thread_new ();
    ti->queue = self;
    
    cape_log_msg (CAPE_LL_TRACE, "CAPE", "queue start", "start new thread");
    
    cape_thread_start (ti->thread, cape_queue__worker__thread, self);
    
    cape_list_push_back (self->threads, ti);
  }
  
  return CAPE_ERR_NONE;
}

//-----------------------------------------------------------------------------

void cape_queue_add (CapeQueue self, CapeSync sync, cape_queue_cb_fct on_event, cape_queue_cb_fct on_done, void* ptr)
{
  CapeQueueItem item = CAPE_NEW (struct CapeQueueItem_s);
  
  item->on_done = on_done;
  item->on_event = on_event;
  item->ptr = ptr;
  item->sync = sync;
  
  cape_mutex_lock (self->mutex);
  
  cape_list_push_back (self->queue, item);
  
  cape_mutex_unlock (self->mutex);

  cape_sync_inc (sync);
  
  sem_post (&(self->sem));
}

//-----------------------------------------------------------------------------

int cape_queue_next (CapeQueue self)
{
  int ret = TRUE;
  CapeQueueItem item = NULL;
 
  sem_wait (&(self->sem));
    
  cape_mutex_lock (self->mutex);
  
  ret = !self->terminated;
  
  if (ret)
  {
    item = cape_list_pop_front (self->queue);
  }
  
  cape_mutex_unlock (self->mutex);
  
  if (item && ret)
  {
    if (item->on_event)
    {
      item->on_event (item->ptr);
    }
    
    cape_queue__item__on_del (item);
  }
  
  return ret;
}

//-----------------------------------------------------------------------------
