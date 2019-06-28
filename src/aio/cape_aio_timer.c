#include "cape_aio_timer.h"

#include "sys/cape_types.h"

//*****************************************************************************

#if defined __BSD_OS || defined __LINUX_OS

//-----------------------------------------------------------------------------

#if defined __BSD_OS

#include <sys/event.h>

#elif defined __LINUX_OS

#include <sys/timerfd.h>
#include <bits/time.h>

#endif

//-----------------------------------------------------------------------------

#include <memory.h>
#include <unistd.h>

//-----------------------------------------------------------------------------

struct CapeAioTimer_s
{
  CapeAioHandle aioh;
  
  void* ptr;
  
  fct_cape_aio_timer_onEvent onEvent;
  
#if defined __BSD_OS

  number_t timeout;
  
#elif defined __LINUX_OS

  number_t handle;
  
#endif
};

//-----------------------------------------------------------------------------

CapeAioTimer cape_aio_timer_new (void* handle)
{
  CapeAioTimer self = CAPE_NEW(struct CapeAioTimer_s);
  
#if defined __BSD_OS

  self->timeout = 0;

#elif defined __LINUX_OS

  self->handle = 0;

#endif
  
  self->ptr = NULL;
  self->onEvent = NULL;
  
  self->aioh = NULL;
  
  return self;
}

//-----------------------------------------------------------------------------

int cape_aio_timer_set (CapeAioTimer self, long timeoutInMs, void* ptr, fct_cape_aio_timer_onEvent fct, CapeErr err)
{
  
#if defined __BSD_OS

  self->timeout = timeoutInMs;
  
#elif defined __LINUX_OS
  
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
  
#endif

  self->ptr = ptr;
  self->onEvent = fct;
  
  return CAPE_ERR_NONE;
}

//-----------------------------------------------------------------------------

static int __STDCALL cape_aio_timer_onEvent (void* ptr, void* handle, int hflags, unsigned long events, void* overlapped, unsigned long param1)
{
  int res = TRUE;
  
  CapeAioTimer self = ptr;
  
#if defined __LINUX_OS

  {
    long value;
    read (self->handle, &value, 8);
  }
  
#endif
  
  if (self->onEvent)
  {
    res = self->onEvent (self->ptr);
  }
  
#if defined __LINUX_OS

  return res ? CAPE_AIO_READ : CAPE_AIO_DONE;
  
#elif defined __BSD_OS

  return res ? CAPE_AIO_TIMER : CAPE_AIO_DONE;

#endif
}

//-----------------------------------------------------------------------------

static void __STDCALL cape_aio_timer_onUnref (void* ptr, CapeAioHandle aioh)
{
  CapeAioTimer self = ptr;
  
#if defined __LINUX_OS

  close (self->handle);

#endif
  
  // delete the AIO handle
  cape_aio_handle_del (&(self->aioh));
  
  CAPE_DEL (&self, struct CapeAioTimer_s);
}

//-----------------------------------------------------------------------------

int cape_aio_timer_add (CapeAioTimer* p_self, CapeAioContext aio)
{
  CapeAioTimer self = *p_self;
  
  *p_self = NULL;
  
#if defined __BSD_OS

  self->aioh = cape_aio_handle_new (self, CAPE_AIO_TIMER, self, cape_aio_timer_onEvent, cape_aio_timer_onUnref);

  cape_aio_context_add (aio, self->aioh, self->timeout);

#else
  
  self->aioh = cape_aio_handle_new ((void*)self->handle, CAPE_AIO_READ, self, cape_aio_timer_onEvent, cape_aio_timer_onUnref);
  
  cape_aio_context_add (aio, self->aioh, 0);

#endif
  
  return 0;
}

//*****************************************************************************



#endif

//-----------------------------------------------------------------------------
