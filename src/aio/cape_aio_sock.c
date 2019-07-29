#include "cape_aio_sock.h"
#include "cape_aio_ctx.h"
#include "cape_aio_timer.h"

// cape includes
#include "sys/cape_types.h"
#include "sys/cape_log.h"
#include "sys/cape_mutex.h"
#include "stc/cape_list.h"

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
#define CAPE_NO_SIGNALS 0

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
  if (*p_self)
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

void cape_aio_socket_close (CapeAioSocket self, CapeAioContext aio)
{
  if (self)
  {
    if (self->aioh)
    {
      cape_aio_context_mod (aio, self->aioh, CAPE_AIO_DONE, 0);
    }
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

            // decrease ref counter (this was increased in send function) 
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
          //cape_log_msg (CAPE_LL_TRACE, "CAPE", "aio_sock", "-- UNREF --");
          
          // decrease ref counter (this was increased in send function) 
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
            
            //cape_log_msg (CAPE_LL_TRACE, "CAPE", "aio_sock", "-- UNREF --");
            
            // decrease ref counter (this was increased in send function) 
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

  // check for errors on the socket, eg connection was refused
  getsockopt(sock, SOL_SOCKET, SO_ERROR, &so_err, &size);

  if (so_err)
  {
      CapeErr err = cape_err_new ();
      
      cape_err_formatErrorOS (err, so_err);
    
      cape_log_fmt (CAPE_LL_ERROR, "CAPE", "socket on_event", "error on socket: %s", cape_err_text(err));
    
      cape_err_del (&err);
      
      // we still have a buffer in the queue
      // this means we have called send before and the ref counter was increased
      if (self->send_buflen)
      {
        //cape_log_msg (CAPE_LL_TRACE, "CAPE", "aio_sock", "-- UNREF --");

        // decrease ref counter (this was increased in send function) 
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

static void __STDCALL cape_aio_socket_onUnref (void* ptr, CapeAioHandle aioh, int force_close)
{
  CapeAioSocket self = ptr;
  
  cape_log_msg (CAPE_LL_TRACE, "CAPE", "aio_sock", "unref");

  if (self->send_buflen)
  {
    cape_log_msg (CAPE_LL_TRACE, "CAPE", "aio_sock", "unref buflen");

    self->send_buflen = 0;
    
    // decrease ref counter (this was increased in send function) 
    cape_aio_socket_unref (self);
  }
  
  cape_aio_socket_unref (self);
}

//-----------------------------------------------------------------------------

void cape_aio_socket_markSent (CapeAioSocket self, CapeAioContext aio)
{
  if (self->mask == CAPE_AIO_NONE)
  {
    if (self->aioh)
    {
      cape_aio_context_mod (aio, self->aioh, CAPE_AIO_WRITE | CAPE_AIO_READ, 0);
    }
  }
  else
  {
    self->mask |= CAPE_AIO_WRITE;
  }
}

//-----------------------------------------------------------------------------

void cape_aio_socket_send (CapeAioSocket self, CapeAioContext aio, const char* bufdata, unsigned long buflen, void* userdata)
{
  // check if we are ready to send
  if (self->send_buflen)
  {
    // increase the refcounter to ensure that the object will nont be deleted during sending cycle
    cape_log_msg (CAPE_LL_ERROR, "CAPE", "aio_sock", "socket has already a buffer to send");    
    return;
  }
  
  // only allow data with a length
  if (buflen == 0)
  {
    if (self->onSent)
    {
      // transfer userdata to the ownership beyond the callback
      self->send_userdata = NULL;
      
      // userdata can be deleted
      self->onSent (self->ptr, self, userdata);                
    }    
    
    // increase the refcounter to ensure that the object will nont be deleted during sending cycle
    cape_log_msg (CAPE_LL_WARN, "CAPE", "aio_sock", "can't send a buffer with buflen = 0");    
    return;
  }
  
  self->send_bufdat = bufdata;
  self->send_buflen = buflen;
  
  self->send_buftos = 0;  
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
        // our object was not added to the AIO subsystem
        // -> try to free it
        // TODO: maybe try again?
        //cape_log_msg (CAPE_LL_TRACE, "CAPE", "aio_sock", "-- UNREF --");

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
  //cape_log_msg (CAPE_LL_TRACE, "CAPE", "aio_sock", "-- INREF --");
  
  cape_aio_socket_inref (self);
}

//-----------------------------------------------------------------------------

void cape_aio_socket_listen (CapeAioSocket* p_self, CapeAioContext aio)
{
  CapeAioSocket self = *p_self;
  
  *p_self = NULL;

  if (self->aioh)
  {
    cape_aio_context_mod (aio, self->aioh, CAPE_AIO_READ, 0);
  }
  else
  {
    self->aioh = cape_aio_handle_new (self->handle, CAPE_AIO_READ, self, cape_aio_socket_onEvent, cape_aio_socket_onUnref);
   
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
  
  // set the socket to none blocking
  {
    int flags = fcntl(sock, F_GETFL, 0);
    if (flags == -1)
    {
      
    }

    flags |= O_NONBLOCK;
    
    if (fcntl(sock, F_SETFL, flags) != 0)
    {
      
    }
  }
  
  if (self->onConnect)
  {
    self->onConnect (self->ptr, (void*)sock, remoteAddr);
  }
  
  return hflags;  
}

//-----------------------------------------------------------------------------

static void __STDCALL cape_aio_accept_onUnref (void* ptr, CapeAioHandle aioh, int force_close)
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

#elif defined __WINDOWS_OS

#include <windows.h>

//-----------------------------------------------------------------------------

struct CapeAioSocket_s
{
    HANDLE handle;
    
    CapeAioHandle aioh;

    int mask;
    
    // callbacks
    
    void* ptr;
    
    fct_cape_aio_socket_onSent onSent;
    
    fct_cape_aio_socket_onRecv onRecv;
    
    fct_cape_aio_socket_onDone onDone;

    // for sending
    
    const char* send_bufdat;
    
    WORD send_buflen;
    
    WORD send_buftos;
    
    void* send_userdata;

    // for receive
    
    char* recv_bufdat;
    
    WORD recv_buflen;
    
    int refcnt;
};

//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------

struct CapeAioAccept_s
{
  SOCKET handle;

  SOCKET asock;
  
  CapeAioHandle aioh;
  
  void* ptr;
  
  fct_cape_aio_accept_onConnect onConnect;
  
  fct_cape_aio_accept_onDone onDone;

  char buffer[1024];
};

//-----------------------------------------------------------------------------

static const DWORD lenAddr = sizeof (struct sockaddr_in) + 16;

//-----------------------------------------------------------------------------

CapeAioAccept cape_aio_accept_new (void* handle)
{
  CapeAioAccept self = CAPE_NEW(struct CapeAioAccept_s);

  self->handle = (SOCKET)handle;
  self->asock = INVALID_SOCKET;

  self->aioh = NULL;
  
  self->ptr = NULL;
  self->onConnect = NULL;
  self->onDone = NULL;
  
  return self;
}

//-----------------------------------------------------------------------------

void cape_aio_accept_del (CapeAioAccept* p_self)
{
  if (*p_self)
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
}

//-----------------------------------------------------------------------------

void cape_aio_accept_callback (CapeAioAccept self, void* ptr, fct_cape_aio_accept_onConnect onConnect, fct_cape_aio_accept_onDone onDone)
{
  self->ptr = ptr;
  self->onConnect = onConnect;
  self->onDone = onDone;
}

//-----------------------------------------------------------------------------

void cape_aio_accept__activate (CapeAioAccept self)
{
  DWORD outBUflen = 0; //1024 - ((sizeof (sockaddr_in) + 16) * 2);
  DWORD dwBytes = 0;
  
  // create a client socket in advance
  self->asock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  if (self->asock == INVALID_SOCKET)
  {
    return;
    //return TError (_ERRCODE_NET__SOCKET_ERROR, "error code: " + IntToStr (::WSAGetLastError()));
  }
  
  memset (self->buffer, 0, 1024);

  // accept connections, client socket will be assigned
  AcceptEx (self->handle, self->asock, self->buffer, outBUflen, lenAddr, lenAddr, &dwBytes, (LPOVERLAPPED)(self->aioh));
}

//-----------------------------------------------------------------------------

static int __STDCALL cape_aio_accept_onEvent (void* ptr, void* handle, int hflags, unsigned long events, void* overlapped, unsigned long param1)
{
  CapeAioAccept self = ptr;

  struct sockaddr *pLocal = NULL, *pRemote = NULL;
  int nLocal = 0, nRemote = 0;
  const char* remoteAddr = NULL;
  
  // retrieve the remote address
  GetAcceptExSockaddrs (self->buffer, 0, lenAddr, lenAddr, &pLocal, &nLocal, &pRemote, &nRemote);
  
  if (pRemote->sa_family == AF_INET)
  {
    remoteAddr = inet_ntoa(((struct sockaddr_in*)pRemote)->sin_addr);
  }
  
  if (self->onConnect)
  {
    // call the callback method
    self->onConnect (self->ptr, (void*)self->asock, remoteAddr);
  }
  
  self->asock = INVALID_SOCKET;
  
  // continue
  cape_aio_accept__activate (self);
  
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

  // create a new AIO handle
  self->aioh = cape_aio_handle_new ((void*)(self->handle), CAPE_AIO_READ, self, cape_aio_accept_onEvent, cape_aio_accept_onUnref);
  
  // add the handle to the AIO subsystem
  cape_aio_context_add (aio, self->aioh, 0);

  // activate the socket for listening
  cape_aio_accept__activate (self);

  *p_self = NULL;
}

//-----------------------------------------------------------------------------

#endif

//-----------------------------------------------------------------------------

struct CapeAioSocketCache_s
{
  CapeAioContext aio_ctx;
  
  CapeAioSocket aio_socket;
  
  CapeList cache;
  
  CapeMutex mutex;
  
  // for callback

  void* ptr;
  
  fct_cape_aio_socket_cache__on_recv on_recv;
  
  int auto_reconnect;
  
  fct_cape_aio_socket_cache__on_event on_retry;
  fct_cape_aio_socket_cache__on_event on_connect;
};

//-----------------------------------------------------------------------------

static void __STDCALL cape_aio_socket_cache__cache_on_del (void* ptr)
{
  CapeStream s = ptr; cape_stream_del (&s);
}

//-----------------------------------------------------------------------------

CapeAioSocketCache cape_aio_socket_cache_new (CapeAioContext aio_ctx)
{
  CapeAioSocketCache self = CAPE_NEW (struct CapeAioSocketCache_s);
  
  self->aio_ctx = aio_ctx;
  self->aio_socket = NULL;
  
  self->cache = cape_list_new (cape_aio_socket_cache__cache_on_del);
  
  self->mutex = cape_mutex_new ();
  
  self->ptr = NULL;  

  self->on_recv = NULL;
  self->on_retry = NULL;
  self->on_connect = NULL;
  
  self->auto_reconnect = FALSE;

  return self;
}

//-----------------------------------------------------------------------------

static void __STDCALL cape_aio_socket_cache__on_done__delete_only (void* ptr, void* userdata)
{
  if (userdata)
  {
    CapeStream s = userdata; cape_stream_del (&s);    
  }

  cape_log_fmt (CAPE_LL_TRACE, "CAPE", "aio_cache done", "cache all cleared");
}

//-----------------------------------------------------------------------------

void cape_aio_socket_cache__close (CapeAioSocketCache self)
{
  CapeAioSocket sock;
  
  cape_mutex_lock (self->mutex);

  sock = self->aio_socket;
  self->aio_socket = NULL;
  
  cape_mutex_unlock (self->mutex);
  
  if (sock)
  {
    cape_log_fmt (CAPE_LL_TRACE, "CAPE", "aio_cache close", "[%p] close connection process initiated", sock->handle);
    
    // disable all callbacks
    cape_aio_socket_callback (sock, NULL, NULL, NULL, cape_aio_socket_cache__on_done__delete_only);
    
    // close the socket and disconnect
    cape_aio_socket_close (sock, self->aio_ctx);
  }
}

//-----------------------------------------------------------------------------

void cape_aio_socket_cache_del (CapeAioSocketCache* p_self)
{
  if (*p_self)
  {
    CapeAioSocketCache self = *p_self;

    cape_aio_socket_cache__close (self);

    // clear the list and free it
    cape_list_del (&(self->cache));
    
    // cleanup the mutex
    cape_mutex_del (&(self->mutex));

    // free memory
    CAPE_DEL (p_self, struct CapeAioSocketCache_s);
  }
}

//-----------------------------------------------------------------------------

static void __STDCALL cape_aio_socket_cache__on_sent (void* ptr, CapeAioSocket socket, void* userdata)
{
  CapeAioSocketCache self = ptr;
  
  // local objects
  CapeStream s = NULL;
  int first_on_sent = FALSE;
  
  // check for userdata
  if (userdata)
  {
    // userdata is always a stream
    s = userdata;
    
    // cleanup stream
    cape_stream_del (&s);    
  }

  cape_mutex_lock (self->mutex);
  
  if (self->aio_socket == NULL)
  {
    // set the socket AIO handle now
    self->aio_socket = socket;
    
    // this is the first write aknoledgement
    first_on_sent = TRUE;
  }
  
  cape_mutex_unlock (self->mutex);

  if (first_on_sent)
  {
    cape_log_fmt (CAPE_LL_TRACE, "CAPE", "aio_cache sent", "[%p] *** CONNECTED ***", socket->handle);
    
    if (self->on_connect)
    {
      // call callback
      self->on_connect (self->ptr);
    }
  }
  
  cape_mutex_lock (self->mutex);

  s = cape_list_pop_front (self->cache);
  
  cape_mutex_unlock (self->mutex);
  
  if (s)
  {
    // if we do have a stream send it to the socket
    cape_aio_socket_send (self->aio_socket, self->aio_ctx, cape_stream_get (s), cape_stream_size (s), s);   
  }    
}

//-----------------------------------------------------------------------------

static void __STDCALL cape_aio_socket_cache__on_recv (void* ptr, CapeAioSocket socket, const char* bufdat, number_t buflen)
{
  CapeAioSocketCache self = ptr;

  if (self->on_recv)
  {
    self->on_recv (self->ptr, bufdat, buflen);    
  }
}

//-----------------------------------------------------------------------------

int __STDCALL cape_aio_socket_cache__on_timer (void* ptr)
{
  CapeAioSocketCache self = ptr;
  
  if (self->on_retry)
  {
    self->on_retry (self->ptr);
  }
  
  return FALSE;
}

//-----------------------------------------------------------------------------

static void __STDCALL cape_aio_socket_cache__on_done (void* ptr, void* userdata)
{
  CapeAioSocketCache self = ptr;
  int retry;
  
  if (userdata)
  {
    CapeStream s = userdata; cape_stream_del (&s);    
  }
  
  cape_mutex_lock (self->mutex);

  if (self->aio_socket)
  {
    cape_log_fmt (CAPE_LL_TRACE, "CAPE", "aio_cache done", "[%p] *** CONNECTION LOST ***", self->aio_socket->handle);

    retry = self->auto_reconnect;    
  }
  else
  {
    cape_log_fmt (CAPE_LL_TRACE, "CAPE", "aio_cache done", "[none] *** CONNECTION LOST ***");    
    
    retry = FALSE;
  }
  
  // disable connection
  self->aio_socket = NULL;

  // clear the cache
  cape_list_clr (self->cache);
  
  cape_mutex_unlock (self->mutex);
  
  // check for auto reconnect system
  if (retry)
  {
    CapeErr err = cape_err_new ();
    
    CapeAioTimer timer = cape_aio_timer_new ();
    
    number_t timeout_in_ms = 10000;
    
    cape_log_fmt (CAPE_LL_TRACE, "CAPE", "aio_cache set", "start retry timer [%ims]", timeout_in_ms);
    
    int res = cape_aio_timer_set (timer, timeout_in_ms, self, cape_aio_socket_cache__on_timer, err);   // create timer with 10 seconds
    if (res)
    {
      cape_log_fmt (CAPE_LL_DEBUG, "CAPE", "aio socket", "can't create reconnect timer: %s", cape_err_text (err));
    }
    
    // add the timer to AIO system
    cape_aio_timer_add (&timer, self->aio_ctx);
    
    cape_err_del (&err);
  }  
}

//-----------------------------------------------------------------------------

void cape_aio_socket_cache_set (CapeAioSocketCache self, void* handle, void* ptr, fct_cape_aio_socket_cache__on_recv on_recv, fct_cape_aio_socket_cache__on_event on_retry, fct_cape_aio_socket_cache__on_event on_connect)
{
  // create a new handler for the created socket
  CapeAioSocket sock = cape_aio_socket_new (handle);
  
  cape_aio_socket_cache__close (self);

  cape_log_fmt (CAPE_LL_TRACE, "CAPE", "aio_cache set", "[%p] register new connection", sock->handle);
  
  // set callback
  cape_aio_socket_callback (sock, self, cape_aio_socket_cache__on_sent, cape_aio_socket_cache__on_recv, cape_aio_socket_cache__on_done);
    
  cape_mutex_lock (self->mutex);

  // set the new socket handler
  self->aio_socket = NULL;

  // set callback
  self->ptr = ptr;
  self->auto_reconnect = FALSE;
  
  self->on_recv = on_recv;
  self->on_retry = on_retry;
  self->on_connect = on_connect;
  
  cape_mutex_unlock (self->mutex);

  {
    CapeAioSocket sock_reference = sock;
    
    // listen on the connection  
    // TODO: provide a method which can do both
    cape_aio_socket_listen (&sock, self->aio_ctx);
    
    // also enable writing to the connection
    cape_aio_socket_markSent (sock_reference, self->aio_ctx);
  }
}

//-----------------------------------------------------------------------------

void cape_aio_socket_cache_retry (CapeAioSocketCache self, int auto_reconnect)
{
  cape_mutex_lock (self->mutex);
  
  self->auto_reconnect = auto_reconnect;
  
  cape_mutex_unlock (self->mutex);
}

//-----------------------------------------------------------------------------

void cape_aio_socket_cache_clr (CapeAioSocketCache self)
{  
  cape_log_msg (CAPE_LL_TRACE, "CAPE", "aio_cache clr", "start reset");
  
  cape_aio_socket_cache__close (self);
  
  cape_mutex_lock (self->mutex);
    
  // clear the cache
  cape_list_clr (self->cache);
  
  cape_mutex_unlock (self->mutex);
}

//-----------------------------------------------------------------------------

int cape_aio_socket_cache_send_s (CapeAioSocketCache self, CapeStream* p_stream, CapeErr err)
{
  int res;
  
  if (*p_stream)
  {
    cape_mutex_lock (self->mutex);
    
    if (self->aio_socket)
    {
      // add the stream to the send cache
      cape_list_push_back (self->cache, (void*)(*p_stream));
      
      // unset the stream
      *p_stream = NULL;
      
      res = CAPE_ERR_NONE;
    }
    else
    {
      // free the stream
      cape_stream_del (p_stream);

      // set the error
      res = cape_err_set (err, CAPE_ERR_NO_OBJECT, "socket is not connected");
    }
    
    cape_mutex_unlock (self->mutex);
  }  
  
  if (res == CAPE_ERR_NONE)
  {
    cape_aio_socket_markSent (self->aio_socket, self->aio_ctx);
  }
  
  return res;
}

//-----------------------------------------------------------------------------

int cape_aio_socket_cache_active (CapeAioSocketCache self)
{
  int active = FALSE;
  
  cape_mutex_lock (self->mutex);

  active = self->aio_socket != NULL;
  
  cape_mutex_unlock (self->mutex);
  
  return active;
}

//-----------------------------------------------------------------------------
