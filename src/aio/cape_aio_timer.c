#include "cape_aio_timer.h"

#include "sys/cape_types.h"

//-----------------------------------------------------------------------------

#include <malloc.h>
#include <sys/timerfd.h>
#include <memory.h>
#include <unistd.h>
#include <bits/time.h>

//-----------------------------------------------------------------------------

struct CapeAioTimer_s
{
  
  long handle;
  
  CapeAioHandle aioh;
  
  void* ptr;
  
  fct_cape_aio_timer_onEvent onEvent;
};

//-----------------------------------------------------------------------------

CapeAioTimer cape_aio_timer_new (void* handle)
{
  CapeAioTimer self = CAPE_NEW(struct CapeAioTimer_s);
  
  self->handle = 0;

  self->ptr = NULL;
  self->onEvent = NULL;
  
  self->aioh = NULL;

  
  return self;
}

//-----------------------------------------------------------------------------

int cape_aio_timer_set (CapeAioTimer self, long timeoutInMs, void* ptr, fct_cape_aio_timer_onEvent fct, CapeErr err)
{
  self->handle = timerfd_create (CLOCK_MONOTONIC, TFD_NONBLOCK);
  
  if (self->handle == -1)
  {
    return cape_err_lastOSError (err);
  }
  
  // set timer interval
  {
    struct timespec ts;

    struct itimerspec newValue;
    struct itimerspec oldValue;
    
    memset (&newValue, 0, sizeof(newValue));  
    memset (&oldValue, 0, sizeof(oldValue));

    // convert from milliseconds to timespec
    ts.tv_sec   =  timeoutInMs / 1000;
    ts.tv_nsec  = (timeoutInMs % 1000) * 1000000;
    
    newValue.it_value = ts; 
    newValue.it_interval = ts;
    
    if (timerfd_settime (self->handle, 0, &newValue, &oldValue) < 0)
    {
      return cape_err_lastOSError (err);
    }
  }
  
  self->ptr = ptr;
  self->onEvent = fct;
  
  return CAPE_ERR_NONE;
}

//-----------------------------------------------------------------------------

static int __STDCALL cape_aio_timer_onEvent (void* ptr, void* handle, int hflags, unsigned long events, void* overlapped, unsigned long param1)
{
  int res = TRUE;
  
  CapeAioTimer self = ptr;
  
  long value;
  read (self->handle, &value, 8);
  
  if (self->onEvent)
  {
    res = self->onEvent (self->ptr);
  }
  
  return res ? CAPE_AIO_READ : CAPE_AIO_DONE;
}

//-----------------------------------------------------------------------------

static void __STDCALL cape_aio_timer_onUnref (void* ptr, CapeAioHandle aioh)
{
  CapeAioTimer self = ptr;
  
  close (self->handle);
  
  // delete the AIO handle
  cape_aio_handle_del (&(self->aioh));
  
  CAPE_DEL (&self, struct CapeAioTimer_s);
}

//-----------------------------------------------------------------------------

int cape_aio_timer_add (CapeAioTimer* p_self, CapeAioContext aio)
{
  CapeAioTimer self = *p_self;
  
  *p_self = NULL;
  
  self->aioh = self->aioh = cape_aio_handle_new ((void*)self->handle, CAPE_AIO_READ, self, cape_aio_timer_onEvent, cape_aio_timer_onUnref);
  
  cape_aio_context_add (aio, self->aioh);
  
  return 0;
}

//-----------------------------------------------------------------------------

