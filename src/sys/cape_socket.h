#ifndef __CAPE_SYS__SOCKET__H
#define __CAPE_SYS__SOCKET__H 1

#include "sys/cape_export.h"
#include "sys/cape_err.h"

//=============================================================================

__CAPE_LIBEX   void*         cape_sock_reader_new     (const char* host, long port, CapeErr err);

__CAPE_LIBEX   void*         cape_sock_acceptor_new   (const char* host, long port, CapeErr err);

//-----------------------------------------------------------------------------

#endif
