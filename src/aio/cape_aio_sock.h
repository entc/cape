#ifndef __CAPE_AIO__SOCK__H
#define __CAPE_AIO__SOCK__H 1

//=============================================================================

#include "sys/cape_export.h"
#include "sys/cape_err.h"
#include "aio/cape_aio_ctx.h"
#include "stc/cape_stream.h"

#include <sys/types.h>

//=============================================================================

struct CapeAioSocket_s; typedef struct CapeAioSocket_s* CapeAioSocket;

//-----------------------------------------------------------------------------

__CAPE_LIBEX   CapeAioSocket     cape_aio_socket_new           (void* handle);

__CAPE_LIBEX   void              cape_aio_socket_inref         (CapeAioSocket);

__CAPE_LIBEX   void              cape_aio_socket_unref         (CapeAioSocket);

__CAPE_LIBEX   void              cape_aio_socket_listen        (CapeAioSocket*, CapeAioContext);

__CAPE_LIBEX   void              cape_aio_socket_markSent      (CapeAioSocket, CapeAioContext);           // INFO: can be called from everywhere

__CAPE_LIBEX   void              cape_aio_socket_close         (CapeAioSocket, CapeAioContext);

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

__CAPE_LIBEX   CapeAioAccept        cape_aio_accept_new            (void* handle);

__CAPE_LIBEX   void                 cape_aio_accept_del            (CapeAioAccept*);

//-----------------------------------------------------------------------------

typedef void       (__STDCALL *fct_cape_aio_accept_onConnect)  (void* ptr, void* handle, const char* remote_host);

typedef void       (__STDCALL *fct_cape_aio_accept_onDone)     (void* ptr);

//-----------------------------------------------------------------------------

__CAPE_LIBEX   void                 cape_aio_accept_callback       (CapeAioAccept, void*, fct_cape_aio_accept_onConnect, fct_cape_aio_accept_onDone);

__CAPE_LIBEX   void                 cape_aio_accept_add            (CapeAioAccept*, CapeAioContext);

//=============================================================================

struct CapeAioSocketCache_s; typedef struct CapeAioSocketCache_s* CapeAioSocketCache;

typedef void    (__STDCALL *fct_cape_aio_socket_cache__on_recv)    (void* ptr, const char* bufdat, number_t buflen);

//-----------------------------------------------------------------------------

__CAPE_LIBEX   CapeAioSocketCache   cape_aio_socket_cache_new      (CapeAioContext);

__CAPE_LIBEX   void                 cape_aio_socket_cache_del      (CapeAioSocketCache*);

__CAPE_LIBEX   void                 cape_aio_socket_cache_set      (CapeAioSocketCache, void* handle, void* ptr, fct_cape_aio_socket_cache__on_recv);

__CAPE_LIBEX   int                  cape_aio_socket_cache_send_s   (CapeAioSocketCache, CapeStream* p_stream, CapeErr err);

//=============================================================================

#endif
