#ifndef __CAPE_AIO__FILE__H
#define __CAPE_AIO__FILE__H 1

#include "sys/cape_export.h"
#include "aio/cape_aio_ctx.h"

//=============================================================================

struct CapeAioFileReader_s; typedef struct CapeAioFileReader_s* CapeAioFileReader;

//-----------------------------------------------------------------------------

__CAPE_LIBEX   CapeAioFileReader  cape_aio_freader_new           (void* handle);

__CAPE_LIBEX   int                cape_aio_freader_add           (CapeAioFileReader*, CapeAioContext);

//=============================================================================

#endif

