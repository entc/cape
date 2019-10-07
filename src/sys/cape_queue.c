#include "cape_queue.h"

//-----------------------------------------------------------------------------

// cape includes
#include "sys/cape_types.h"
#include "sys/cape_log.h"
#include "sys/cape_mutex.h"
#include "sys/cape_thread.h"
#include "stc/cape_list.h"

// linux includes
#include <sys/eventfd.h>
#include <unistd.h>

//-----------------------------------------------------------------------------

struct CapeQueue_s
{
  CapeMutex mutex;
  
  int fdevent;
  
  CapeList threads;
  
  CapeList queue;
  
  int locked;
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

  write (self->queue->fdevent, &(uint64_t){20}, sizeof(uint64_t));

  cape_thread_join (self->thread);
  
  cape_thread_del (&(self->thread));  
}

//-----------------------------------------------------------------------------

struct CapeQueueItem_s
{
  cape_queue_cb_fct on_event;
  
  cape_queue_cb_fct on_done;
  
  void* ptr;
  
}; typedef struct CapeQueueItem_s* CapeQueueItem;

//-----------------------------------------------------------------------------

void __STDCALL cape_queue__item__on_del (void* ptr)
{
  CapeQueueItem item = ptr;
  
  if (item->on_done)
  {
    item->on_done (item->ptr);
  }
  
  CAPE_DEL (&item, struct CapeQueueItem_s);
}

//-----------------------------------------------------------------------------

CapeQueue cape_queue_new (void)
{
  CapeQueue self = CAPE_NEW (struct CapeQueue_s);
  
  self->mutex = cape_mutex_new ();
  self->fdevent = eventfd (0, 0);
  
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
        
    // remove all queued items
    cape_list_clr (self->queue);
    
    cape_list_del (&(self->threads));
    
    cape_list_del (&(self->queue));
    
    cape_mutex_del (&(self->mutex));

    close (self->fdevent);
    
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

void cape_queue_add (CapeQueue self, cape_queue_cb_fct on_event, cape_queue_cb_fct on_done, void* ptr)
{
  cape_mutex_lock (self->mutex);
  
  {
    CapeQueueItem item = CAPE_NEW (struct CapeQueueItem_s);
    
    item->on_done = on_done;
    item->on_event = on_event;
    item->ptr = ptr;
    
    cape_list_push_back (self->queue, item);
  }
  
  cape_mutex_unlock (self->mutex);
  
  write (self->fdevent, &(uint64_t){1}, sizeof(uint64_t));
}

//-----------------------------------------------------------------------------

int cape_queue_next (CapeQueue self)
{
  int ret = TRUE;
  CapeQueueItem item = NULL;

  uint64_t v;
  
  read (self->fdevent, &v, sizeof(v));
  
  printf ("%li\n", v);
  
  cape_mutex_lock (self->mutex);
  
  {
    number_t queue_size = cape_list_size (self->queue);
    
    if (queue_size)
    {
      item = cape_list_pop_front (self->queue);
    }
    else
    {
      ret = FALSE;
    }
  }
  
  {
    number_t queue_size = cape_list_size (self->queue);    
  }
  
  cape_mutex_unlock (self->mutex);
  
  if (item)
  {
    if (item->on_event)
    {
      item->on_event (item->ptr);
    }
    
    if (item->on_done)
    {
      item->on_done (item->ptr);
    }
    
    CAPE_DEL (&item, struct CapeQueueItem_s);
  }
  
  return ret;
}

//-----------------------------------------------------------------------------

void cape_queue_wait (CapeQueue self)
{
  
  
}

//-----------------------------------------------------------------------------
