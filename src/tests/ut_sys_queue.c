#include "sys/cape_queue.h"
#include "sys/cape_log.h"
#include "sys/cape_thread.h"

static int cnt = 0;

//-----------------------------------------------------------------------------

static void __STDCALL cape_queue01_callback (void* ptr)
{
  int h = ++cnt;
  
  cape_log_fmt (CAPE_LL_DEBUG, "CAPE", "UT :: queue", "[%i] callback start", h);

  cape_thread_sleep (1000);
  
  cape_log_fmt (CAPE_LL_DEBUG, "CAPE", "UT :: queue", "[%i] callback done", h);
}

//-----------------------------------------------------------------------------

// c includes
#include <stdio.h>

int main (int argc, char *argv[])
{
  CapeErr err = cape_err_new ();
  
  CapeQueue queue01 = cape_queue_new ();
  
  // this creates 10 threads
  cape_queue_start (queue01, 5, err);


  // add 10 tasks
  {
    int i;
    
    for (i = 0; i < 10; i++)
    {
      cape_queue_add (queue01, cape_queue01_callback, NULL, NULL);      
    }
  }
  
  cape_thread_sleep (100);
  
  cape_queue_wait (queue01);
  
exit_and_cleanup:

  // this shall stop all threads
  cape_queue_del (&queue01);
  
  if (cape_err_code(err))
  {
    cape_log_fmt (CAPE_LL_ERROR, "CAPE", "UT :: queue", "error: %s", cape_err_text(err));
    
    return 1;
  }
  else
  {
    return 0;
  }
}

//-----------------------------------------------------------------------------
