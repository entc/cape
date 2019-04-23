#include "cape_aio_ctx.h" 

// CAPE includes
#include "sys/cape_types.h"
#include "sys/cape_err.h"
#include "stc/cape_list.h"
#include "sys/cape_log.h"

//*****************************************************************************

#if defined __MS_IOCP

#include <windows.h>



//*****************************************************************************

#elif defined __BSD_OS || defined __LINUX_OS

#if defined __BSD_OS

#include <sys/event.h>

#else

#include <sys/epoll.h>
#include <sys/signalfd.h>

#define CAPE_AIO_EPOLL_MAXEVENTS 1

#endif

#include <pthread.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <signal.h>
#include <stdlib.h>

//-----------------------------------------------------------------------------

struct CapeAioHandle_s
{
  
  void* hfd;
  
  void* ptr;
  
  int hflags;
  
  fct_cape_aio_onUnref on_unref;
  
  fct_cape_aio_onEvent on_event;
  
};

//-----------------------------------------------------------------------------

CapeAioHandle cape_aio_handle_new (void* handle, int hflags, void* ptr, fct_cape_aio_onEvent on_event, fct_cape_aio_onUnref on_unref)
{
  CapeAioHandle self = CAPE_NEW (struct CapeAioHandle_s);
  
  self->hfd = handle;
  self->ptr = ptr;
  
  self->on_event = on_event;
  self->on_unref = on_unref;
  
  self->hflags = hflags;
  
  return self;
}

//-----------------------------------------------------------------------------

void cape_aio_handle_del (CapeAioHandle* p_self)
{
  if (*p_self)
  {
    CAPE_DEL (p_self, struct CapeAioHandle_s);
  }
}

//-----------------------------------------------------------------------------

struct CapeAioContext_s
{
  
#if defined __BSD_OS
  
  int kq;               // kevent file descriptor

#else
  
  int efd;              // epoll file descriptor
  
  long sfd;             // eventfd file descriptor
  
  int smap[32];         // map for signal handling
  
#endif
  
  CapeList events;      // store all events into this list (used only for destruction)
  
  pthread_mutex_t mutex;
};

//-----------------------------------------------------------------------------

void cape_aio_handle_unref (CapeAioHandle self)
{
  if (self->on_unref)
  {
    // call the callback to signal the destruction of the handle
    self->on_unref (self->ptr, self);
  }
}

//-----------------------------------------------------------------------------

static void __STDCALL cape_aio_context_events_onDestroy (void* ptr)
{
  cape_aio_handle_unref (ptr);
}

//-----------------------------------------------------------------------------

CapeAioContext cape_aio_context_new (void)
{
  CapeAioContext self = CAPE_NEW (struct CapeAioContext_s);
  
  memset (&(self->mutex), 0, sizeof(pthread_mutex_t));
  
  pthread_mutex_init (&(self->mutex), NULL);
  
#if defined __BSD_OS

  self->kq = -1;

#else

  self->efd = -1;
  self->sfd = -1;

  {
    int i;
    
    for (i = 0; i < 32; i++)
    {
      self->smap[i] = 0;
    }
  }
  
#endif
    
  self->events = cape_list_new (cape_aio_context_events_onDestroy);
  
  return self;
}

//-----------------------------------------------------------------------------

void cape_aio_context_closeAll (CapeAioContext self)
{
  
#if defined __BSD_OS

  if (self->kq >= 0)
  {
    close (self->kq);
    self->kq = -1;
  }
  
#else
  
  if (self->efd >= 0)
  {
    close (self->efd);
    
    self->efd = -1;
  }

#endif
  
  pthread_mutex_lock(&(self->mutex));
  
  if (self->events)
  {
    cape_list_del (&(self->events));
  }
  
  pthread_mutex_unlock(&(self->mutex));
  
  printf ("close all done\n");
}

//-----------------------------------------------------------------------------

void cape_aio_context_del (CapeAioContext* p_self)
{
  CapeAioContext self = *p_self;
  
  cape_aio_context_closeAll (self);
  
  pthread_mutex_destroy (&(self->mutex));
  
  CAPE_DEL (p_self, struct CapeAioContext_s);
}

//-----------------------------------------------------------------------------

int cape_aio_context_open (CapeAioContext self, CapeErr err)
{
#if defined __BSD_OS

  // create a new kevent
  self->kq = kqueue ();
  
  // check if the open was successful
  if (self->kq == -1)
  {
    return cape_err_lastOSError (err);
  }

#else
  
  // create a new epol
  self->efd = epoll_create1 (0);
  
  // check if the open was successful
  if (self->efd == -1)
  {
    return cape_err_lastOSError (err);
  }

#endif
  
  return CAPE_ERR_NONE;
}

//-----------------------------------------------------------------------------

int cape_aio_context_close (CapeAioContext self, CapeErr err)
{
  // create an user event
  //return cape_aio_context_add (self, NULL, 0, self, cape_aio_context_close_onEvent, NULL);
  
  //printf ("send internal SIGTERM\n");
  
  kill (getpid(), SIGTERM);
  
  return CAPE_ERR_NONE;
}

//-----------------------------------------------------------------------------

int cape_aio_context_wait (CapeAioContext self, CapeErr err)
{
  while (cape_aio_context_next (self, -1, err) == CAPE_ERR_NONE);
  
  return CAPE_ERR_NONE;
}

//-----------------------------------------------------------------------------

void cape_aio_remove_handle (CapeAioContext self, CapeAioHandle hobj)
{
  void* ptr = NULL;
  
  // enter monitor
  pthread_mutex_lock (&(self->mutex));
  
  // try to find the handle (expensive)
  // TODO: we might want to use a map for storing the handles
  {
    CapeListCursor cursor; cape_list_cursor_init (self->events, &cursor, CAPE_DIRECTION_FORW);
    
    while (cape_list_cursor_next (&cursor))
    {
      CapeAioHandle h = cape_list_node_data (cursor.node);
      
      if (h == hobj)
      {
        // extract the ptr, otherwise a deadlock can apear
        // if in the on_del method another handle will be added to the AIO system
        ptr = cape_list_node_extract (self->events, cursor.node);
        
        goto exit_and_unlock;
      }
    }
  }
  
exit_and_unlock:
  
  // leave monitor
  pthread_mutex_unlock (&(self->mutex));
  
  if (ptr)
  {
    // decrease referenced counted content
    cape_aio_handle_unref (ptr);
  }
}

//-----------------------------------------------------------------------------

#if defined __BSD_OS

#else

void cape_aio_update_events (struct epoll_event* event, int hflags)
{
  
  event->events = EPOLLET | EPOLLONESHOT;
  
  if (hflags & CAPE_AIO_READ)
  {
    event->events |= EPOLLIN;
  }
  
  if (hflags & CAPE_AIO_WRITE)
  {
    event->events |= EPOLLOUT;
  }
  
  if (hflags & CAPE_AIO_ALIVE)
  {
    event->events |= EPOLLHUP;
  }
}

//-----------------------------------------------------------------------------

int cape_aio_context_sigmask (CapeAioContext self, sigset_t* sigset)
{
  int res, i;
  
  // null the sigset
  res = sigemptyset (sigset);
  
  for (i = 0; i < 32; i++)
  {
    if (self->smap[i])
    {
      // add this signal to the sigset
      res = sigaddset (sigset, i);
    }
  }
  
  return 0;
}

#endif

//-----------------------------------------------------------------------------

int cape_aio_context_next (CapeAioContext self, long timeout_in_ms, CapeErr err)
{
#if defined __BSD_OS

  int res;
  struct timespec tmout;
  
  struct kevent event;
  memset (&event, 0x0, sizeof(struct kevent));
  
  if (timeout_in_ms == -1)
  {
    res = kevent (self->kq, NULL, 0, &event, 1, NULL);
  }
  else
  {
    tmout.tv_sec = timeout_in_ms / 1000;
    tmout.tv_nsec = (timeout_in_ms % 1000) * 1000;
    
    res = kevent (self->kq, NULL, 0, &event, 1, &tmout);
  }
  
  if( res == -1 )
  {
    if (errno == EINTR)
    {
      return CAPE_ERR_NONE;
    }
    
    return cape_err_lastOSError (err);
  }
  else if (event.flags & EV_ERROR)
  {
    return cape_err_lastOSError (err);
  }
  else if (res == 0)
  {
    return CAPE_ERR_NONE;  // timeout
  }
  else
  {
    // retrieve the handle object from the userdata of the epoll event
    CapeAioHandle hobj = event.udata;
    if (hobj)
    {
      number_t hflags_result;
      
      if (hobj->on_event)
      {
        hflags_result = hobj->on_event (hobj->ptr, (void*)hobj->hfd, hobj->hflags, event.flags, NULL, 0);
      }
      else
      {
        hflags_result = 0;
      }
      
      if (hflags_result & CAPE_AIO_DONE)
      {
        
        
        // remove the handle from events
        cape_aio_remove_handle (self, hobj);
        
        //printf ("[%p] handle removed\n", self);
        
      }
      
      if (hflags_result & CAPE_AIO_ABORT)
      {
        return CAPE_ERR_CONTINUE;
      }
      
      if (hflags_result == CAPE_AIO_NONE)
      {
        
      }
      else
      {
        
      }
    }
    else
    {
      const char* signalKind = NULL;
      
      // assign all known signals
      switch (event.ident)
      {
        case 1: signalKind = "SIGHUP (Hangup detected on controlling terminal or death of controlling process)"; break;
        case 2: signalKind = "SIGINT (Interrupt from keyboard)"; break;
        case 3: signalKind = "SIGQUIT (Quit from keyboard)"; break;
        case 4: signalKind = "SIGILL (Illegal Instruction)"; break;
        case 6: signalKind = "SIGABRT (Abort signal from abort(3))"; break;
        case 8: signalKind = "SIGFPE (Floating-point exception)"; break;
        case 9: signalKind = "SIGKILL (Kill signal)"; break;
        case 11: signalKind = "SIGSEGV (Invalid memory reference)"; break;
        case 13: signalKind = "SIGPIPE (Broken pipe: write to pipe with no readers; see pipe(7))"; break;
        case 15: signalKind = "SIGTERM (Termination signal)"; break;
      }
      
      if (signalKind)
      {
        cape_log_fmt (CAPE_LL_TRACE, "CAPE", "{aio next}", "signal seen [%i] -> %s", event.ident, signalKind);
        
        if (event.ident == SIGINT || event.ident == SIGTERM)
        {
          return CAPE_ERR_CONTINUE;
        }
      }
      else
      {
        //eclog_fmt (LL_TRACE, "ENTC", "signal", "signal seen [%i] -> unknown signal", event.ident);
      }
    }
    
    return CAPE_ERR_NONE;
  }
  
#else
  
  int n, res;
  struct epoll_event *events;
  sigset_t sigset;
  
  res = cape_aio_context_sigmask (self, &sigset);
  // we must block the signals in order for signalfd to receive them
  //res = sigprocmask (SIG_BLOCK, &sigset, NULL);
  
  // we should also block sigpipe
  sigaddset (&sigset, SIGPIPE);
  
  events = calloc (CAPE_AIO_EPOLL_MAXEVENTS, sizeof(struct epoll_event));
  
  if (events == NULL)
  {
    return -1;
  }
  
  //printf ("[%p] wait for next event\n", self);
  
  n = epoll_pwait (self->efd, events, CAPE_AIO_EPOLL_MAXEVENTS, timeout_in_ms, &sigset);
  
  //printf ("[%p] GOT EVENT %i\n", self, n);
  
  if (n < 0)
  {
    res = cape_err_lastOSError (err);    
    goto exit;
  }
  
  if (n > 0)
  {
    int i = 0;
    
    // retrieve the handle object from the userdata of the epoll event
    CapeAioHandle hobj = events[i].data.ptr;
    if (hobj)
    {
      number_t hflags_result;
      
      if (hobj->on_event)
      {
        hflags_result = hobj->on_event (hobj->ptr, (void*)hobj->hfd, hobj->hflags, events[i].events, NULL, 0);
      }
      else
      {
        hflags_result = 0;
      }
      
      if (hflags_result & CAPE_AIO_DONE)
      {
        epoll_ctl (self->efd, EPOLL_CTL_DEL, (long)(hobj->hfd), &(events[i]));
        
        // remove the handle from events
        cape_aio_remove_handle (self, hobj);
        
        //printf ("[%p] handle removed\n", self);
        
        goto exit;
      }
      
      if (hflags_result & CAPE_AIO_ABORT)
      {
        res = CAPE_ERR_CONTINUE;
      }
      
      if (hflags_result == CAPE_AIO_NONE)
      {
        cape_aio_update_events (&(events[i]), hobj->hflags);
        
        epoll_ctl (self->efd, EPOLL_CTL_MOD, (long)(hobj->hfd), &(events[i]));
      }
      else
      {
        cape_aio_update_events (&(events[i]), hflags_result);
        
        hobj->hflags = hflags_result;
        
        epoll_ctl (self->efd, EPOLL_CTL_MOD, (long)(hobj->hfd), &(events[i]));
      }
    }
  }
  
  //---------------------
exit:
  
  // finally cleanup
  free (events);
  return res;

#endif
}

//-----------------------------------------------------------------------------

void cape_aio_context_mod (CapeAioContext self, CapeAioHandle aioh, int hflags)
{
#if defined __BSD_OS


#else
  struct epoll_event event;
  
  // set the user pointer to the handle
  event.data.ptr = aioh;
  
  cape_aio_update_events (&event, hflags);
  
  int s = epoll_ctl (self->efd, EPOLL_CTL_MOD, (long)(aioh->hfd), &event);
  if (s < 0)
  {
    int errCode = errno;
    if (errCode == EPERM)
    {
      printf ("this filedescriptor is not supported by epoll\n");
      
    }
    else
    {
      CapeErr err = cape_err_new ();
      
      cape_err_lastOSError (err);
      
      printf ("can't add fd to epoll: %s\n", cape_err_text (err));
      
      cape_err_del (&err);
    }
    
    return;
  }
  
#endif
  
  aioh->hflags = hflags;
}

//-----------------------------------------------------------------------------

int cape_aio_context_add (CapeAioContext self, CapeAioHandle aioh)
{
#if defined __BSD_OS
  
  int res;
  
  struct kevent kev;
  memset (&kev, 0x0, sizeof(struct kevent));

  EV_SET (&kev, (long)aioh->hfd, EVFILT_READ | EVFILT_WRITE, EV_ADD | EV_ENABLE | EV_ONESHOT, 0, 0, (void*)aioh);
  
  res = kevent (self->kq, &kev, 1, NULL, 0, NULL);
  if (res < 0)
  {
    CapeErr err = cape_err_new ();
    
    cape_err_lastOSError (err);
    
    printf ("can't add fd [%li] to epoll: %s\n", (long)aioh->hfd, cape_err_text (err));
    
    cape_err_del (&err);

    return FALSE;
  }
  
#else
  
  if (self->efd < 0)
  {
    cape_aio_handle_unref (aioh);
    
    return FALSE;
  }
  
  struct epoll_event event;
  
  // set the user pointer to the handle
  event.data.ptr = aioh;
  
  cape_aio_update_events (&event, aioh->hflags);
  
  int s = epoll_ctl (self->efd, EPOLL_CTL_ADD, (long)(aioh->hfd), &event);
  if (s < 0)
  {
    int errCode = errno;
    if (errCode == EPERM)
    {
      printf ("this filedescriptor is not supported by epoll\n");
      
    }
    else
    {
      CapeErr err = cape_err_new ();
      
      cape_err_lastOSError (err);
      
      printf ("can't add fd [%li] to epoll: %s\n", (long)aioh->hfd, cape_err_text (err));
      
      cape_err_del (&err);
    }
    
    return FALSE;
  }
  
#endif

  pthread_mutex_lock (&(self->mutex));
  
  cape_list_push_back (self->events, aioh);
  
  pthread_mutex_unlock (&(self->mutex));

  return TRUE;
}

#if defined __LINUX_OS

//-----------------------------------------------------------------------------

int cape_aio_context_signal_map (CapeAioContext self, int signal, int status)
{
  if (signal > 0 && signal < 32)
  {
    self->smap[signal] = status;
    
    return 0;
  }
  
  return -1;
}

//-----------------------------------------------------------------------------

static int __STDCALL cape_aio_context_signal_onEvent (void* ptr, void* handle, int hflags, unsigned long events, void* overlapped, unsigned long param1)
{
  CapeAioContext self = ptr;
  
  struct signalfd_siginfo info;
  
  ssize_t bytes = read ((long)handle, &info, sizeof(info));
  
  if (bytes == sizeof(info))
  {
    int sig = info.ssi_signo;
    
    //printf ("SIGNAL SEEN %i\n", sig);
    
    // send again (otherwise other signal handlers will not be triggered)
    kill (getpid(), sig);
    
    if (sig > 0 && sig < 32)
    {
      return self->smap[sig] | CAPE_AIO_READ;
    }
  }
  
  return CAPE_AIO_NONE;
}

//-----------------------------------------------------------------------------

static void __STDCALL cape_aio_context_signal_onUnref (void* ptr, CapeAioHandle aioh)
{
  //printf ("close signalfs\n");
  
  close ((long)(aioh->hfd));
  
  cape_aio_handle_del (&aioh);
}

#endif

//-----------------------------------------------------------------------------

int cape_aio_context_set_interupts (CapeAioContext self, int sigint, int term, CapeErr err)
{
#if defined __BSD_OS

  int res;
  
  if (sigint)
  {
    struct kevent kev;
    memset (&kev, 0x0, sizeof (struct kevent));
    
    EV_SET (&kev, SIGINT, EVFILT_SIGNAL, EV_ADD | EV_ENABLE, 0, 0, NULL);
    
    // register SIGINT signal
    res = kevent (self->kq, &kev, 1, NULL, 0, NULL);
    if (res < 0)
    {
      return cape_err_lastOSError (err);
    }
  }

  if (term)
  {
    struct kevent kev;
    memset (&kev, 0x0, sizeof (struct kevent));
    
    EV_SET (&kev, SIGTERM, EVFILT_SIGNAL, EV_ADD | EV_ENABLE, 0, 0, NULL);
    
    // register SIGTERM signal
    res = kevent (self->kq, &kev, 1, NULL, 0, NULL);
    if (res < 0)
    {
      return cape_err_lastOSError (err);
    }
  }

#else
  
  int res;
  sigset_t sigset;
  
  if (self->sfd != -1)
  {
    return cape_err_set (err, CAPE_ERR_NO_OBJECT, "file-descriptor is not set");
  }

  if (sigint)
  {
    cape_aio_context_signal_map (self, SIGINT, CAPE_AIO_ABORT);
  }
  
  if (term)
  {
    cape_aio_context_signal_map (self, SIGTERM, CAPE_AIO_ABORT);
  }
  
  res = cape_aio_context_sigmask (self, &sigset);
  if (res)
  {
    return cape_err_lastOSError (err);    
  }
  
  // we must block the signals in order for signalfd to receive them
  res = sigprocmask (SIG_BLOCK, &sigset, NULL);
  if (res)
  {
    return cape_err_lastOSError (err);    
  }
  
  // create the signalfd
  self->sfd = signalfd(-1, &sigset, 0);
  
  CapeAioHandle aioh = cape_aio_handle_new ((void*)self->sfd, CAPE_AIO_READ, self, cape_aio_context_signal_onEvent, cape_aio_context_signal_onUnref);
  
  // add the signalfd to the event context
  cape_aio_context_add (self, aioh);
  
#endif

  return CAPE_ERR_NONE;
}

//-----------------------------------------------------------------------------

#endif
