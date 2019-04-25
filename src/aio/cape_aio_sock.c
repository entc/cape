#include "cape_aio_sock.h"

// cape includes
#include "sys/cape_types.h"
#include "sys/cape_log.h"

#if defined __BSD_OS || defined __LINUX_OS

#include <memory.h>
#include <sys/socket.h>	// basic socket definitions
#include <sys/types.h>
#include <arpa/inet.h>	// inet(3) functions
#include <sys/fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <netdb.h>

// includes specific event subsystem
#if defined __BSD_OS

#include <sys/event.h>
#ifdef SO_NOSIGPIPE
#define CAPE_NO_SIGNALS SO_NOSIGPIPE
#else
#define CAPE_NO_SIGNALS 0
#endif

#elif defined __LINUX_OS

#include <sys/epoll.h>
#define CAPE_NO_SIGNALS MSG_NOSIGNAL

#endif

//-----------------------------------------------------------------------------

struct CapeAioSocket_s
{
  
    void* handle;
    
    CapeAioHandle aioh;

    int mask;
    
    // callbacks
    
    void* ptr;
    
    fct_cape_aio_socket_onSent onSent;
    
    fct_cape_aio_socket_onRecv onRecv;
    
    fct_cape_aio_socket_onDone onDone;

    // for sending
    
    const char* send_bufdat;
    
    ssize_t send_buflen;
    
    ssize_t send_buftos;
    
    void* send_userdata;

    // for receive
    
    char* recv_bufdat;
    
    ssize_t recv_buflen;
    
    int refcnt;
    
};

//-----------------------------------------------------------------------------

CapeAioSocket cape_aio_socket_new (void* handle)
{
  CapeAioSocket self = CAPE_NEW(struct CapeAioSocket_s);
  
  self->handle = handle;
  self->aioh = NULL;
  
  self->mask = CAPE_AIO_NONE;

  // sending  
  self->send_bufdat = NULL;
  self->send_buflen = 0;
  self->send_buftos = 0;
  self->send_userdata = NULL;
  
  // receiving
  self->recv_bufdat = NULL;
  self->recv_buflen = 0;
  
  // callbacks
  self->ptr = NULL;
  self->onSent = NULL;
  self->onRecv = NULL;
  self->onDone = NULL;
  
  // set none blocking
  {
    int flags = fcntl((long)self->handle, F_GETFL, 0);
    
    flags |= O_NONBLOCK;
    
    fcntl((long)self->handle, F_SETFL, flags);
  }
  
  self->refcnt = 1;
  
  return self;
}

//-----------------------------------------------------------------------------

void cape_aio_socket_del (CapeAioSocket* p_self)
{
  CapeAioSocket self = *p_self;
  
  if (self->onDone)
  {
    self->onDone (self->ptr, self->send_userdata);
    
    self->send_userdata = NULL;
  }
  
  if (self->recv_bufdat)
  {
    free (self->recv_bufdat);
  }
  
  // turn off wait timeout of the socket
  {
    struct linger sl;
    
    sl.l_onoff = 1;     // active linger options
    sl.l_linger = 0;    // set timeout to zero
    
    // apply linger option to kernel and socket
    setsockopt((long)self->handle, SOL_SOCKET, SO_LINGER, &sl, sizeof(sl));
  }
    
  // signal that this side of the socket is not going to continue to write or read from the socket
  shutdown ((long)self->handle, SHUT_RDWR);
    
  // close the handle
  close ((long)self->handle);
  
  // delete the AIO handle
  cape_aio_handle_del (&(self->aioh));

  CAPE_DEL (p_self, struct CapeAioSocket_s);
}

//-----------------------------------------------------------------------------

void cape_aio_socket_inref (CapeAioSocket self)
{
#ifdef __GCC_HAVE_SYNC_COMPARE_AND_SWAP_16

  __sync_add_and_fetch (&(self->refcnt), 1);
  
#else
  
  (self->refcnt)++;
  
#endif
}

//-----------------------------------------------------------------------------

void cape_aio_socket_unref (CapeAioSocket self)
{
#ifdef __GCC_HAVE_SYNC_COMPARE_AND_SWAP_16
  
  int val = (__sync_sub_and_fetch (&(self->refcnt), 1));
  
#else
  
  int val = --(self->refcnt);
  
#endif
  
  if (val == 0)
  {
    cape_aio_socket_del (&self);
  }
}

//-----------------------------------------------------------------------------

void cape_aio_socket_callback (CapeAioSocket self, void* ptr, fct_cape_aio_socket_onSent onSent, fct_cape_aio_socket_onRecv onRecv, fct_cape_aio_socket_onDone onDone)
{
    self->ptr = ptr;
    
    self->onSent = onSent;
    self->onRecv = onRecv;
    self->onDone = onDone;
}

//-----------------------------------------------------------------------------

void cape_aio_socket_read (CapeAioSocket self, long sockfd)
{
    // initial the buffer
    if (self->recv_bufdat == NULL)        
    {
        self->recv_bufdat = malloc (1024);
        self->recv_buflen = 1024;
    }
    
    while (TRUE)
    {
      ssize_t readBytes = recv (sockfd, self->recv_bufdat, self->recv_buflen, CAPE_NO_SIGNALS);
      if (readBytes < 0)
      {
        if( (errno != EWOULDBLOCK) && (errno != EINPROGRESS) && (errno != EAGAIN))
        {
          cape_log_fmt (CAPE_LL_ERROR, "CAPE", "socket read", "error while writing data to the socket [%i]", errno);
          self->mask |= CAPE_AIO_DONE;
        }
        
        return;
      }
      else if (readBytes == 0)
      {
        //printf ("CONNECTION RESET BY PEER\n");
        
        // disable all other read / write / etc mask flags
        //otherwise we will run into a race condition
        self->mask = CAPE_AIO_DONE;
        
        return;
      }
      else
      { 
        //printf ("%i bytes recved\n", readBytes);
        
        // we got data -> dump it
        if (self->onRecv)
        {
          self->onRecv (self->ptr, self, self->recv_bufdat, readBytes);
        }
      }
    }    
}

//-----------------------------------------------------------------------------

void cape_aio_socket_write (CapeAioSocket self, long sockfd)
{
    if (self->send_buflen == 0)
    {
        // disable to listen on write events
        self->mask &= ~CAPE_AIO_WRITE;
        
        if (self->onSent)
        {
            // userdata must be empty, if not we don't it still needs to be sent, so don't delete it here
            self->onSent (self->ptr, self, NULL);
        }
    }
    else
    {
      while (self->send_bufdat)
      {
        ssize_t writtenBytes = send (sockfd, self->send_bufdat + self->send_buftos, self->send_buflen - self->send_buftos, CAPE_NO_SIGNALS);
        if (writtenBytes < 0)
        {
          if( (errno != EWOULDBLOCK) && (errno != EINPROGRESS) && (errno != EAGAIN))
          {
            CapeErr err = cape_err_new ();
            
            cape_err_formatErrorOS (err, errno); 
            
            cape_log_fmt (CAPE_LL_ERROR, "CAPE", "socket write", "error while writing data to the socket: %s", cape_err_text(err));
            
            self->mask |= CAPE_AIO_DONE;
            
            cape_err_del(&err);

            cape_aio_socket_unref (self);
            
            return;
          }
          else
          {
            // TODO: make it better
            
            // do a context switch
            sleep (0);
            
            continue;
          }
        }
        else if (writtenBytes == 0)
        {
          cape_aio_socket_unref (self);
          
          // disable all other read / write / etc mask flags
          //otherwise we will run into a race condition
          self->mask = CAPE_AIO_DONE;
          
          return;
        }
        else
        {
          self->send_buftos += writtenBytes;
          
          if (self->send_buftos == self->send_buflen)
          {
            self->mask &= ~CAPE_AIO_WRITE;
            
            self->send_buflen = 0;
            self->send_bufdat = NULL;
            
            if (self->onSent)
            {
              // create a local copy, because in the callback self->send_userdata might be reset
              void* userdata = self->send_userdata;
              
              // transfer userdata to the ownership beyond the callback
              self->send_userdata = NULL;
              
              // userdata can be deleted
              self->onSent (self->ptr, self, userdata);                
            }
            
            cape_aio_socket_unref (self);
          }
        }
      }
    }
}

//-----------------------------------------------------------------------------

static int __STDCALL cape_aio_socket_onEvent (void* ptr, void* handle, int hflags, unsigned long events, void* overlapped, unsigned long param1)
{
  CapeAioSocket self = ptr;
  
  self->mask = hflags; 
  
  long sock = (long)handle;
    
  int so_err; socklen_t size = sizeof(int); 

  printf ("ON EVENT\n");
  
  // check for errors on the socket, eg connection was refused
  getsockopt(sock, SOL_SOCKET, SO_ERROR, &so_err, &size);

  if (so_err)
  {
      CapeErr err = cape_err_new ();
      
      cape_err_formatErrorOS (err, so_err);
    
      cape_log_fmt (CAPE_LL_ERROR, "CAPE", "socket on_event", "error on socket: %s", cape_err_text(err));
    
      cape_err_del (&err);
      
      // we still have a buffer in the queue
      if (self->send_buflen)
      {
        cape_aio_socket_unref (self);
      }
      
      self->mask |= CAPE_AIO_DONE;        
  }
  else
  {
#ifdef __BSD_OS
    if (events & EVFILT_READ)
#else
    if (events & EPOLLIN)
#endif
      {
          cape_aio_socket_read (self, sock);
      }
      
#ifdef __BSD_OS
    if (events & EVFILT_WRITE)
#else
      if (events & EPOLLOUT)
#endif
      {
          cape_aio_socket_write (self, sock);
      }
  }
  

  {
      int ret = self->mask;
      
      self->mask = CAPE_AIO_NONE;
      
      return ret;
  }
}

//-----------------------------------------------------------------------------

static void __STDCALL cape_aio_socket_onUnref (void* ptr, CapeAioHandle aioh)
{
  cape_aio_socket_unref (ptr);    
}

//-----------------------------------------------------------------------------

void cape_aio_socket_markSent (CapeAioSocket self, CapeAioContext aio)
{
  printf ("mark\n");

  if (self->mask == CAPE_AIO_NONE)
  {
    if (self->aioh)
    {
      printf ("mark update\n");
      
      cape_aio_context_mod (aio, self->aioh, CAPE_AIO_WRITE | CAPE_AIO_READ, 0);
    }
  }
  else
  {
    printf ("mark return\n");

    self->mask |= CAPE_AIO_WRITE;
  }
}

//-----------------------------------------------------------------------------

void cape_aio_socket_send (CapeAioSocket self, CapeAioContext aio, const char* bufdata, unsigned long buflen, void* userdata)
{
  self->send_bufdat = bufdata;
  self->send_buflen = buflen;
  
  self->send_buftos = 0;
  
  printf ("send package %p\n", userdata);
  
  self->send_userdata = userdata;
    
  if (self->mask == CAPE_AIO_NONE)
  {
    // correct epoll flags for this filedescriptor
    if (self->aioh)
    {
      cape_aio_context_mod (aio, self->aioh, CAPE_AIO_WRITE | CAPE_AIO_READ, 0);
    }
    else
    {
      // create a new AIO handle
      self->aioh = cape_aio_handle_new (self->handle, CAPE_AIO_WRITE, self, cape_aio_socket_onEvent, cape_aio_socket_onUnref);
      
      // register handle at the AIO system
      if (!cape_aio_context_add (aio, self->aioh, 0))
      {
        cape_aio_socket_unref (self);
        
        return;
      }
    }
  }
  else
  {
    self->mask |= CAPE_AIO_WRITE;
  }

  // increase the refcounter to ensure that the object will nont be deleted during sending cycle
  cape_aio_socket_inref (self);
}

//-----------------------------------------------------------------------------

void cape_aio_socket_listen (CapeAioSocket* p_self, CapeAioContext aio)
{
  CapeAioSocket self = *p_self;
  
  *p_self = NULL;

  if (self->aioh)
  {
    cape_aio_context_mod (aio, self->aioh, CAPE_AIO_WRITE | CAPE_AIO_READ, 0);
  }
  else
  {
    printf ("create handle\n");
    
    self->aioh = cape_aio_handle_new (self->handle, CAPE_AIO_READ | CAPE_AIO_WRITE, self, cape_aio_socket_onEvent, cape_aio_socket_onUnref);
   
    cape_aio_context_add (aio, self->aioh, 0);
  }
}

//-----------------------------------------------------------------------------

struct CapeAioAccept_s
{
  void* handle;
  
  CapeAioHandle aioh;
  
  void* ptr;
  
  fct_cape_aio_accept_onConnect onConnect;
  
  fct_cape_aio_accept_onDone onDone;
};

//-----------------------------------------------------------------------------

CapeAioAccept cape_aio_accept_new (void* handle)
{
  CapeAioAccept self = CAPE_NEW(struct CapeAioAccept_s);

  self->handle = handle;
  self->aioh = NULL;
  
  self->ptr = NULL;
  self->onConnect = NULL;
  self->onDone = NULL;
  
  return self;
}

//-----------------------------------------------------------------------------

void cape_aio_accept_del (CapeAioAccept* p_self)
{
  CapeAioAccept self = *p_self;
  
  if (self->onDone)
  {
    self->onDone (self->ptr);
  }
  
  // delete the AIO handle
  cape_aio_handle_del (&(self->aioh));
  
  CAPE_DEL(p_self, struct CapeAioAccept_s);  
}

//-----------------------------------------------------------------------------

void cape_aio_accept_callback (CapeAioAccept self, void* ptr, fct_cape_aio_accept_onConnect onConnect, fct_cape_aio_accept_onDone onDone)
{
  self->ptr = ptr;
  self->onConnect = onConnect;
  self->onDone = onDone;
}

//-----------------------------------------------------------------------------

static int __STDCALL cape_aio_accept_onEvent (void* ptr, void* handle, int hflags, unsigned long events, void* overlapped, unsigned long param1)
{
  CapeAioAccept self = ptr;
  
  struct sockaddr addr;
  socklen_t addrlen = 0;
  
  const char* remoteAddr = NULL;
  
  memset (&addr, 0x00, sizeof(addr));
  
  long sock = accept ((long)(self->handle), &addr, &addrlen);
  if (sock < 0)
  {
    if( (errno != EWOULDBLOCK) && (errno != EINPROGRESS) && (errno != EAGAIN))
    {
      return CAPE_AIO_NONE;
    }
    else
    {
      return hflags;      
    }
  }
  
  remoteAddr = inet_ntoa(((struct sockaddr_in*)&addr)->sin_addr);
  
  if (self->onConnect)
  {
    self->onConnect (self->ptr, (void*)sock, remoteAddr);
  }
  
  return hflags;  
}

//-----------------------------------------------------------------------------

static void __STDCALL cape_aio_accept_onUnref (void* ptr, CapeAioHandle aioh)
{
  CapeAioAccept self = ptr;
  
  cape_aio_accept_del (&self);
}

//-----------------------------------------------------------------------------

void cape_aio_accept_add (CapeAioAccept* p_self, CapeAioContext aio)
{
  CapeAioAccept self = *p_self;
  
  *p_self = NULL;
  
  self->aioh = cape_aio_handle_new (self->handle, CAPE_AIO_READ, self, cape_aio_accept_onEvent, cape_aio_accept_onUnref);
  
  cape_aio_context_add (aio, self->aioh, 0);
}

//-----------------------------------------------------------------------------

#endif
