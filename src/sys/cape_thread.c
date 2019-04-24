#include "cape_thread.h"
#include "sys/cape_types.h"

//-----------------------------------------------------------------------------

#if defined __LINUX_OS || defined __BSD_OS

#include <pthread.h>
#include <sys/wait.h>
#include <unistd.h>

#include "sys/cape_types.h"

//-----------------------------------------------------------------------------------

struct CapeThread_s
{
  cape_thread_worker_fct fct;
  
  void* ptr;
  
  pthread_t tid;
  
  int status;
};

//-----------------------------------------------------------------------------------

static void* cape_thread_run (void* params)
{
  CapeThread self = params;
  
  if (self->fct)
  {
    while (self->fct (self->ptr))
    {
      pthread_testcancel();
      
      wait(0);
    }
  }
  
  return NULL;
}

//-----------------------------------------------------------------------------------

CapeThread cape_thread_new (void)
{
  CapeThread self = CAPE_NEW (struct CapeThread_s);
  
  self->fct = NULL;
  self->ptr = NULL;
  //memset(self->tid, 0x00, sizeof(pthread_t));
  
  self->status = FALSE;
  
  return self;
}

//-----------------------------------------------------------------------------------

void cape_thread_del (CapeThread* pself)
{
  CapeThread self = *pself;
  
  if (self->status)
  {
    // WARNING
    cape_thread_join (self);
  }
  
  CAPE_DEL (pself, struct CapeThread_s);
}

//-----------------------------------------------------------------------------------

void cape_thread_cancel (CapeThread self)
{
  pthread_cancel (self->tid);
}

//-----------------------------------------------------------------------------------

void cape_thread_start (CapeThread self, cape_thread_worker_fct fct, void* ptr)
{
  // define some special attributes
  pthread_attr_t attr;
  pthread_attr_init(&attr);
  pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
  // assign the callback parameters
  self->fct = fct;
  self->ptr = ptr;
  // finally create the thread
  self->status = (pthread_create(&(self->tid), &attr, cape_thread_run, self) == 0);
}

//-----------------------------------------------------------------------------------

void cape_thread_join (CapeThread self)
{
  if (self->status) {
    void* status;
    pthread_join(self->tid, &status);
    
    self->status = FALSE;
  }
}

//-----------------------------------------------------------------------------

void cape_thread_sleep (unsigned long milliseconds)
{
  usleep (milliseconds * 1000);
}

//-----------------------------------------------------------------------------

#elif defined _WIN64 || defined _WIN32

#include <windows.h>

//-----------------------------------------------------------------------------

struct CapeThread_s
{
  HANDLE th;
  
  cape_thread_worker_fct fct;
  
  void* ptr;
};

//-----------------------------------------------------------------------------------

DWORD WINAPI cape_thread_run (LPVOID ptr)
{
  CapeThread self = ptr;
  
  if (self->fct)
  {
    // do the user defined loop
    while (self->fct(self->ptr));
  }
  return 0;
}

//-----------------------------------------------------------------------------------

CapeThread cape_thread_new (void)
{
  CapeThread self = CAPE_NEW (struct CapeThread_s);
  
  self->th = NULL;
  
  return self;
}

//-----------------------------------------------------------------------------------

void cape_thread_del (CapeThread* pself)
{
  CapeThread self = *pself;
  
  cape_thread_join(self);
  
  CAPE_DEL (pself, struct CapeThread_s);
}

//-----------------------------------------------------------------------------------

void cape_thread_start (CapeThread self, cape_thread_callback_fct fct, void* ptr)
{
  if (self->th == NULL)
  {
    self->fct = fct;
    self->ptr = ptr;
    self->th = CreateThread (NULL, 0, (LPTHREAD_START_ROUTINE)cape_thread_run, (LPVOID)self, 0, NULL);
  }
}

//-----------------------------------------------------------------------------------

void cape_thread_join (CapeThread self)
{
  if (self->th != NULL)
  {
    // wait until the thread terminates
    WaitForSingleObject (self->th, INFINITE);
    // release resources
    CloseHandle(self->th);
    self->th = NULL;
  }
}

//-----------------------------------------------------------------------------------

void cape_thread_cancel (CapeThread self)
{
  if (self->th != NULL)
  {
    if (TerminateThread(self->th, 0) == 0)
    {
      EcErr err = ecerr_create ();
      
      ecerr_lastErrorOS (err, ENTC_LVL_ERROR);
      
      eclog_fmt (LL_ERROR, "ENTC", "thread", "can't cancel thread: %s", err->text);
      
      ecerr_destroy (&err);
    }
  }
}

//-----------------------------------------------------------------------------------

void cape_thread_sleep (unsigned long milliseconds)
{
  Sleep (milliseconds);
}

#endif

//-----------------------------------------------------------------------------


