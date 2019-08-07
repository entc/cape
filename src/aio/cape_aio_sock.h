#ifndef __CAPE_AIO__SOCK__H
#define __CAPE_AIO__SOCK__H 1

//=============================================================================

#include "sys/cape_export.h"
#include "sys/cape_err.h"
#include "aio/cape_aio_ctx.h"
#include "stc/cape_stream.h"

#include <sys/types.h>

//=============================================================================

/*
 * \ brief This class implements the socket handler for the AIO subsystem. The constructor takes a handle (socket file descriptor) and registers
           it on the AIO subsystem. All events on the socket can be catched by the callback functions. The socket can react on recv and write events.
           The write events notification will be turned off automatically after a write event got catched. This must be turned on 
           with 'cape_aio_socket_markSent' again. Write event should be considered as ask the system if the write buffer is ready?
           
           This class is referenced counted. If you need a private copy, please use the 'cape_aio_socket_inref' function to ensure 
           the socket will not be deleted in the meanwhile. The 'cape_aio_socket_unref' releases the ownership.
           
           By using the function 'cape_aio_socket_listen' the ownership will move to the AIO subsystem. The object will be released there. If you
           use only the callback functions, it is safe to access the object everytime.
 */

//-----------------------------------------------------------------------------

struct CapeAioSocket_s; typedef struct CapeAioSocket_s* CapeAioSocket;

//-----------------------------------------------------------------------------

__CAPE_LIBEX   CapeAioSocket     cape_aio_socket_new           (void* handle);                           ///< constructor to allocate memory for the object

__CAPE_LIBEX   void              cape_aio_socket_inref         (CapeAioSocket);                          ///< increase the reference counter of the object

__CAPE_LIBEX   void              cape_aio_socket_unref         (CapeAioSocket);                          ///< decrease the reference counter of the object

__CAPE_LIBEX   void              cape_aio_socket_listen        (CapeAioSocket*, CapeAioContext);         ///< turn on 'receive' events on the socket

__CAPE_LIBEX   void              cape_aio_socket_markSent      (CapeAioSocket, CapeAioContext);          ///< turn on 'write' events on the socket

__CAPE_LIBEX   void              cape_aio_socket_close         (CapeAioSocket, CapeAioContext);          ///< turn off all events and disconnect

//-----------------------------------------------------------------------------

typedef void       (__STDCALL *fct_cape_aio_socket_onSent)     (void* ptr, CapeAioSocket socket, void* userdata);
typedef void       (__STDCALL *fct_cape_aio_socket_onRecv)     (void* ptr, CapeAioSocket socket, const char* bufdat, number_t buflen);
typedef void       (__STDCALL *fct_cape_aio_socket_onDone)     (void* ptr, void* userdata);

__CAPE_LIBEX   void                 cape_aio_socket_callback       (CapeAioSocket, void*, fct_cape_aio_socket_onSent, fct_cape_aio_socket_onRecv, fct_cape_aio_socket_onDone);

//-----------------------------------------------------------------------------

// WARNING: can only be used in the onSent callback function, to avoid race-conditions
__CAPE_LIBEX   void                 cape_aio_socket_send           (CapeAioSocket, CapeAioContext, const char* bufdata, unsigned long buflen, void* userdata);   

//=============================================================================

struct CapeAioAccept_s; typedef struct CapeAioAccept_s* CapeAioAccept;

//-----------------------------------------------------------------------------

__CAPE_LIBEX   CapeAioAccept        cape_aio_accept_new            (void* handle);                       ///< constructor to allocate memory for the object

__CAPE_LIBEX   void                 cape_aio_accept_del            (CapeAioAccept*);                     ///< destructor to free memory

//-----------------------------------------------------------------------------

typedef void       (__STDCALL *fct_cape_aio_accept_onConnect)  (void* ptr, void* handle, const char* remote_host);
typedef void       (__STDCALL *fct_cape_aio_accept_onDone)     (void* ptr);

//-----------------------------------------------------------------------------

__CAPE_LIBEX   void                 cape_aio_accept_callback       (CapeAioAccept, void*, fct_cape_aio_accept_onConnect, fct_cape_aio_accept_onDone);

__CAPE_LIBEX   void                 cape_aio_accept_add            (CapeAioAccept*, CapeAioContext);     ///< turn on 'accept' events, ownership moves to AioContext

//=============================================================================

/*
 \ brief This class implements a cached sendbuffer for the AIO socket subsystem. If you are not sure how to use the CapeAioSocket class
           please consider CapeAioSocketCache instead. 
*/

//-----------------------------------------------------------------------------

struct CapeAioSocketCache_s; typedef struct CapeAioSocketCache_s* CapeAioSocketCache;

// callback prototypes
typedef void  (__STDCALL *fct_cape_aio_socket_cache__on_recv)      (void* ptr, const char* bufdat, number_t buflen);
typedef void  (__STDCALL *fct_cape_aio_socket_cache__on_event)     (void* ptr);


//-----------------------------------------------------------------------------

__CAPE_LIBEX  CapeAioSocketCache  cape_aio_socket_cache_new     (CapeAioContext);                                           ///< constructor to allocate memory for the object

__CAPE_LIBEX  void                cape_aio_socket_cache_del     (CapeAioSocketCache*);                                      ///< destructor to free memory

__CAPE_LIBEX  void                cape_aio_socket_cache_set     (CapeAioSocketCache, void* handle, void* ptr, fct_cape_aio_socket_cache__on_recv, fct_cape_aio_socket_cache__on_event on_retry, fct_cape_aio_socket_cache__on_event on_connect);

__CAPE_LIBEX  void                cape_aio_socket_cache_clr     (CapeAioSocketCache);                                       ///< stops all operations (disconnect)

__CAPE_LIBEX  int                 cape_aio_socket_cache_send_s  (CapeAioSocketCache, CapeStream* p_stream, CapeErr err);    ///< sends the content of the stream to the socket

__CAPE_LIBEX  void                cape_aio_socket_cache_retry   (CapeAioSocketCache, int auto_reconnect);

__CAPE_LIBEX  int                 cape_aio_socket_cache_active  (CapeAioSocketCache);                                       ///< returns true if connection is active
  
//=============================================================================

#endif
