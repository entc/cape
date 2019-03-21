#include "cape_socket.h"

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
  struct hostent* server;
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
  
  printf ("listen on [%s:%li]\n", host, port);
  
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

