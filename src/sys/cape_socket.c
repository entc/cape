#include "cape_socket.h"

// cape includes
#include "sys/cape_log.h"

//-----------------------------------------------------------------------------

#if defined __LINUX_OS || defined __BSD_OS

// c includes
#include <memory.h>
#include <sys/socket.h>	// basic socket definitions
#include <sys/types.h>
#include <arpa/inet.h>	// inet(3) functions
#include <sys/fcntl.h>
#include <unistd.h>
#include <netdb.h>

//-----------------------------------------------------------------------------

void cape_socket_set_host (struct sockaddr_in* addr, const char* host, long port)
{
  memset (addr, 0, sizeof(struct sockaddr_in));
  
  addr->sin_family = AF_INET;      // set the network type
  addr->sin_port = htons(port);    // set the port
  
  // set the address
  if (host == NULL)
  {
    addr->sin_addr.s_addr = INADDR_ANY;
  }
  else
  {
    const struct hostent* server = gethostbyname(host);
    
    if(server)
    {
      bcopy((char*)server->h_addr, (char*)&(addr->sin_addr.s_addr), server->h_length);
    }
    else
    {
      addr->sin_addr.s_addr = INADDR_ANY;
    }
  }
}

//-----------------------------------------------------------------------------

void* cape_sock_reader_new (const char* host, long port, CapeErr err)
{
  struct sockaddr_in addr;
  long sock;
  
  cape_socket_set_host (&addr, host, port);
  
  // create socket
  sock = socket (AF_INET, SOCK_STREAM, 0);
  if (sock < 0)
  {
    goto exit;
  }
  
  // make the socket none-blocking
  {
    int flags = fcntl(sock, F_GETFL, 0);
    
    if (flags == -1)
    {
      goto exit;
    }
    
    flags |= O_NONBLOCK;
    
    if (fcntl(sock, F_SETFL, flags) != 0)
    {
      goto exit;
    }
  }
  
  // connect, don't check result because it is none-blocking
  connect (sock, (const struct sockaddr*)&(addr), sizeof(addr));
  
  return (void*)sock;
  
  exit:
  
  // save the last system error into the error object
  cape_err_lastOSError (err);
  
  if (sock >= 0)
  {
    close(sock);    
  }
  
  return NULL;
}

//-----------------------------------------------------------------------------

void* cape_sock_acceptor_new  (const char* host, long port, CapeErr err)
{
  struct sockaddr_in addr;
  long sock = -1;
  
  cape_socket_set_host (&addr, host, port);
  
  // create socket
  sock = socket (AF_INET, SOCK_STREAM, 0);
  if (sock < 0)
  {
    goto exit;
  }
  
  {
    int opt = 1;
    if(setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (char *)&opt, sizeof(opt)) < 0)
    {
      goto exit;
    }
    
    if (bind(sock, (const struct sockaddr*)&(addr), sizeof(addr)) < 0)
    {
      goto exit;
    }
  }
  
  // cannot fail
  listen(sock, SOMAXCONN);
  
  cape_log_fmt (CAPE_LL_TRACE, "CAPE", "cape_socket", "listen on [%s:%li]", host, port);
  
  return (void*)sock;
  
exit:
  
  // save the last system error into the error object
  cape_err_lastOSError (err);
  
  if (sock >= 0)
  {
    close(sock);    
  }
  
  return NULL;
}

//-----------------------------------------------------------------------------

#elif defined _WIN64 || defined _WIN32

#include <ws2tcpip.h>
#include <winsock2.h>

#include <windows.h>
#include <stdio.h>

//-----------------------------------------------------------------------------

static int init_wsa (void)
{
  static WSADATA wsa;
  
  return (WSAStartup (MAKEWORD(2,2), &wsa) == 0);
}

//-----------------------------------------------------------------------------

void* cape_sock_reader_new (const char* host, long port, CapeErr err)
{
  // TODO: needs to be done
}

//-----------------------------------------------------------------------------

void* cape_sock_acceptor_new (const char* host, long port, CapeErr err)
{
  struct addrinfo hints;
  
  // local variables
  struct addrinfo* addr = NULL;
  SOCKET sock = INVALID_SOCKET;

  // in windows the WSA system must be initialized first
  if (!init_wsa ())
  {
    goto exit_and_cleanup;
  }
  
  ZeroMemory(&hints, sizeof(hints));
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_protocol = IPPROTO_TCP;
  hints.ai_flags = AI_PASSIVE;
  
  {
    char buffer[10];
    sprintf_s (buffer, 10, "%u", port);
    
    if (getaddrinfo (host, buffer, &hints, &addr) != 0)
    {
      goto exit_and_cleanup;
    }
  }
  
  sock = socket (addr->ai_family, addr->ai_socktype, addr->ai_protocol);
  if (sock == INVALID_SOCKET)
  {
    goto exit_and_cleanup;
  }
  
  {
    int optVal = TRUE;
    int optLen = sizeof(int);
    
    if (getsockopt (sock, SOL_SOCKET, SO_REUSEADDR, (char*)&optVal, &optLen) != 0)
    {
      goto exit_and_cleanup;
    }
  }
  
  if (bind (sock, addr->ai_addr, (int)addr->ai_addrlen) == SOCKET_ERROR)
  {
    goto exit_and_cleanup;
  }
  
  // in windows this can fail
  if (listen (sock, SOMAXCONN) == SOCKET_ERROR)
  {
    goto exit_and_cleanup;
  }
  
  cape_log_fmt (CAPE_LL_TRACE, "CAPE", "cape_socket", "listen on [%s:%li]", host, port);
  
  freeaddrinfo (addr);
  
  return (void*)sock;
  
exit_and_cleanup:

  // save the last system error into the error object
  cape_err_formatErrorOS (err, WSAGetLastError());

  if (addr)
  {
    freeaddrinfo (addr);
  }
  
  if (sock != INVALID_SOCKET)
  {
    closesocket (sock);
  }
  
  return NULL;
}

//-----------------------------------------------------------------------------

#endif
