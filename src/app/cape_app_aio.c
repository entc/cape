#include "aio/cape_aio_ctx.h"
#include "aio/cape_aio_sock.h"

#include "sys/cape_socket.h"

//-----------------------------------------------------------------------------

#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include <signal.h>
#include <fcntl.h>

//-----------------------------------------------------------------------------

int main (int argc, char *argv[])
{
  int res;
  
  CapeErr err = cape_err_new();
  
  CapeAioContext aio = cape_aio_context_new ();
  
  res = cape_aio_context_open (aio);
  
  res = cape_aio_context_signal_map (aio, SIGINT, CAPE_AIO_ABORT);
  res = cape_aio_context_signal_map (aio, SIGTERM, CAPE_AIO_ABORT);
  
  res = cape_aio_context_signal_set (aio);

  {
    void* sock = cape_sock_reader_new ("127.0.0.1", 40011, err);
    
    if (sock)
    {
        CapeAioSocket s = cape_aio_socket_new (sock);

        cape_aio_socket_listen (&s, aio);
    }
    else
    {
        printf ("can't create socket: %s\n", cape_err_text(err));
    }
      
  }

  res = cape_aio_context_wait (aio);
  
  cape_aio_context_del (&aio);

  cape_err_del (&err);
  
  return 0;
}

//-----------------------------------------------------------------------------

