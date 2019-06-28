#ifdef __GNUC__
#define _GNU_SOURCE 1
#endif

#include "cape_exec.h"

// cape includes
#include "sys/cape_log.h"
#include "sys/cape_types.h"
#include "stc/cape_stream.h"
#include "stc/cape_list.h"

//-----------------------------------------------------------------------------

#if defined __LINUX_OS || defined __BSD_OS

// c includes
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdarg.h>

//-----------------------------------------------------------------------------

struct CapeExec_s
{
  CapeStream stdout_stream;
  
  CapeStream stderr_stream;
  
  CapeList arguments;
};

//-----------------------------------------------------------------------------

#define CAPE_PIPE_READ     0
#define CAPE_PIPE_WRITE    1

//-----------------------------------------------------------------------------

static void __STDCALL cape_exec__arguments__on_del (void* ptr)
{
  CapeString h = ptr; cape_str_del (&h);
}

//-----------------------------------------------------------------------------

CapeExec cape_exec_new (void)
{
  CapeExec self = CAPE_NEW (struct CapeExec_s);
  
  self->stdout_stream = cape_stream_new ();
  self->stderr_stream = cape_stream_new ();
  
  self->arguments = cape_list_new (cape_exec__arguments__on_del);
  
  return self;
}

//-----------------------------------------------------------------------------

void cape_exec_del (CapeExec* p_self)
{
  CapeExec self = *p_self;
  
  cape_stream_del (&(self->stdout_stream));
  cape_stream_del (&(self->stderr_stream));
  
  cape_list_del (&(self->arguments));
  
  CAPE_DEL (p_self, struct CapeExec_s);
}

//-----------------------------------------------------------------------------

void cape_exec_run__execute (CapeExec self, const CapeString executable)
{
  int i = 0;
  
  // construct
  char** arguments = CAPE_ALLOC (sizeof(char*) * (cape_list_size (self->arguments) + 2));

  arguments[i] = (char*)executable;
  
  {
    CapeListCursor cursor; cape_list_cursor_init (self->arguments, &cursor, CAPE_DIRECTION_FORW);

    while (cape_list_cursor_next (&cursor))
    {
      i++;
      arguments[i] = cape_list_node_data (cursor.node);
    }
  }
  
  i++;
  arguments[i] = NULL;
  
  {
    int ret = execvp (executable, arguments);
    
    if (ret == -1)
    {
      CapeErr err = cape_err_new ();
      
      cape_err_lastOSError (err);
      
      printf ("error in execv: %s\n", cape_err_text (err));
      
      cape_err_del (&err);
    }
  }
  
  CAPE_FREE (arguments);
}

//-----------------------------------------------------------------------------

void cape_exec_run__rec_bytes (int fd, CapeStream s)
{
  char buffer [1024];

  while (TRUE)
  {
    number_t bytes_read = read (fd, buffer, 1024);
   
    if (bytes_read > 0)
    {
      cape_stream_append_buf (s, buffer, bytes_read);
    }
    else
    {
      return;
    }    
  }  
}

//-----------------------------------------------------------------------------

int cape_exec_run__recording (CapeExec self, pid_t pid, int fd_stdout, int fd_stderr, CapeErr err)
{
  int res;

  int status = 0;
  pid_t w;
    
  do 
  {
    fd_set fdsread;
    FD_ZERO(&fdsread);

    // add to select set
    FD_SET (fd_stdout, &fdsread);
    FD_SET (fd_stderr, &fdsread);
        
    w = waitpid (pid, &status, WUNTRACED | WCONTINUED);
    
    // run select
    {
      int res_select = select (fd_stderr + 1, &fdsread, NULL, NULL, NULL);
      
      if (res_select == -1)
      {
        res = cape_err_lastOSError (err);        
        return res;
      }
      else
      {
        if (FD_ISSET (fd_stdout, &fdsread))
        {
          cape_exec_run__rec_bytes (fd_stdout, self->stdout_stream);
        }

        if (FD_ISSET (fd_stderr, &fdsread))
        {
          cape_exec_run__rec_bytes (fd_stderr, self->stderr_stream);
        }
      }
    }
  }
  while (!WIFEXITED(status) && !WIFSIGNALED(status));
  
  if (status != 0)
  {
    cape_log_fmt (CAPE_LL_ERROR, "CAPE", "exec - run", "exitcode: %i", status);
    
    res = cape_err_set_fmt (err, CAPE_ERR_RUNTIME, "executed program returned: %i", status);
  }
  else
  {
    res = CAPE_ERR_NONE;
  }
    
  return res;
}

//-----------------------------------------------------------------------------

int cape_exec_run (CapeExec self, const char* executable, CapeErr err)
{
  int res;
  pid_t pid;
  
  int outfd[2], errfd[2];
  
  if (pipe (outfd) == -1)
  {
    res = cape_err_lastOSError (err);
    goto exit_and_cleanup;
  }
  
  if (pipe (errfd) == -1)
  {
    res = cape_err_lastOSError (err);
    goto exit_and_cleanup;
  }

  // fork
  pid = fork();
  
  switch (pid)
  {
    case -1: // error
    {
      res = cape_err_lastOSError (err);
      goto exit_and_cleanup;
    }
    case  0: // child process
    {
      // close reading ends
      close (outfd[CAPE_PIPE_READ]);
      close (errfd[CAPE_PIPE_READ]);
      
      cape_log_fmt (CAPE_LL_TRACE, "CAPE", "exec - run", "execute: %s", executable);
      
      // redirecting stdout
      dup2 (outfd[CAPE_PIPE_WRITE], STDOUT_FILENO);
      
      // redirecting stderr
      dup2 (errfd[CAPE_PIPE_WRITE], STDERR_FILENO);
      
      cape_exec_run__execute (self, executable);
      
      close (outfd[CAPE_PIPE_WRITE]);
      close (errfd[CAPE_PIPE_WRITE]);
      
      exit(0);
    }
    default: // parent process
    {
      close (outfd[CAPE_PIPE_WRITE]);
      close (errfd[CAPE_PIPE_WRITE]);
      
      res = cape_exec_run__recording (self, pid, outfd[CAPE_PIPE_READ], errfd[CAPE_PIPE_READ], err);
      
      close (outfd[CAPE_PIPE_READ]);
      close (errfd[CAPE_PIPE_READ]);
      
      return res;
    }
  }

  res = CAPE_ERR_NONE;
  
exit_and_cleanup:

  close (outfd[CAPE_PIPE_WRITE]);
  close (outfd[CAPE_PIPE_READ]);

  close (errfd[CAPE_PIPE_READ]);
  close (errfd[CAPE_PIPE_WRITE]);
  
  return res;
}

//-----------------------------------------------------------------------------

void cape_exec_append_s (CapeExec self, const char* parameter)
{
  CapeString h = cape_str_cp (parameter);
  
  cape_log_fmt (CAPE_LL_TRACE, "CAPE", "exec - param", "append parameter: %s", h);
    
  cape_list_push_back (self->arguments, h);
}

//-----------------------------------------------------------------------------

void cape_exec_append_fmt (CapeExec self, const char* format, ...)
{
  CapeString parameter = NULL;
  
  va_list ptr;  
  va_start(ptr, format);
  
  parameter = cape_str_flp (format, ptr);
  
  va_end(ptr); 
  
  if (parameter)
  {
    cape_log_fmt (CAPE_LL_TRACE, "CAPE", "exec - param", "append parameter: %s", parameter);
    
    cape_list_push_back (self->arguments, parameter);    
  }  
}

//-----------------------------------------------------------------------------

const CapeString cape_exec_get_stdout (CapeExec self)
{
  return cape_stream_get (self->stdout_stream);
}

//-----------------------------------------------------------------------------

const CapeString cape_exec_get_stderr (CapeExec self)
{
  return cape_stream_get (self->stderr_stream);
}

//-----------------------------------------------------------------------------

#elif defined __WINDOWS_OS

#include <windows.h>

#define BUFSIZE 4096

typedef struct
{
  HANDLE hr;
  HANDLE hw;
  
  //OVERLAPPED stOverlapped;
  
  char buffer[BUFSIZE];
  
  DWORD dwState;
  
  CapeStream stream;
  
  //int initial;
  
} CapePipe;

//-----------------------------------------------------------------------------

struct CapeExec_s
{
  CapeList arguments;

  CapePipe errPipe;
  CapePipe outPipe;
};

//-----------------------------------------------------------------------------

static void __STDCALL cape_exec__arguments__on_del (void* ptr)
{
  CapeString h = ptr; cape_str_del (&h);
}

//-----------------------------------------------------------------------------

CapeExec cape_exec_new (void)
{
  CapeExec self = CAPE_NEW (struct CapeExec_s);
  
  self->arguments = cape_list_new (cape_exec__arguments__on_del);
  
  self->errPipe.hr = NULL;
  self->errPipe.hw = NULL;
  self->outPipe.hr = NULL;
  self->outPipe.hw = NULL;
  
  self->outPipe.stream = cape_stream_new ();
  self->errPipe.stream = cape_stream_new ();

  return self;
}

//-----------------------------------------------------------------------------

void cape_exec_append_s (CapeExec self, const char* parameter)
{
  CapeString h = cape_str_cp (parameter);
  
  cape_log_fmt (CAPE_LL_TRACE, "CAPE", "exec - param", "append parameter: %s", h);
  
  cape_list_push_back (self->arguments, h);
}

//-----------------------------------------------------------------------------

void cape_exec_append_fmt (CapeExec self, const char* format, ...)
{
  CapeString parameter = NULL;
  
  va_list ptr;
  va_start(ptr, format);
  
  parameter = cape_str_flp (format, ptr);
  
  va_end(ptr);
  
  if (parameter)
  {
    cape_log_fmt (CAPE_LL_TRACE, "CAPE", "exec - param", "append parameter: %s", parameter);
    
    cape_list_push_back (self->arguments, parameter);
  }
}

//-----------------------------------------------------------------------------


#endif

//-----------------------------------------------------------------------------
