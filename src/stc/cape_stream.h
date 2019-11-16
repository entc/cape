#ifndef __CAPE_STC__STREAM__H
#define __CAPE_STC__STREAM__H 1

// cape includes
#include "sys/cape_export.h"
#include "sys/cape_types.h"
#include "sys/cape_time.h"
#include "stc/cape_str.h"

//=============================================================================

struct CapeStream_s; typedef struct CapeStream_s* CapeStream;

//-----------------------------------------------------------------------------

__CAPE_LIBEX CapeStream      cape_stream_new (void);

__CAPE_LIBEX void            cape_stream_del (CapeStream*);

__CAPE_LIBEX void            cape_stream_clr (CapeStream);

__CAPE_LIBEX const char*     cape_stream_get (CapeStream);

__CAPE_LIBEX number_t        cape_stream_size (CapeStream);

__CAPE_LIBEX const char*     cape_stream_data (CapeStream);

//-----------------------------------------------------------------------------
// convert to other types

__CAPE_LIBEX CapeString      cape_stream_to_str (CapeStream*);

__CAPE_LIBEX number_t        cape_stream_to_n (CapeStream);

__CAPE_LIBEX CapeString      cape_stream_to_s (CapeStream);

//-----------------------------------------------------------------------------
// append functions

__CAPE_LIBEX void            cape_stream_append_str (CapeStream, const char*);

__CAPE_LIBEX void            cape_stream_append_buf (CapeStream, const char*, unsigned long size);

__CAPE_LIBEX void            cape_stream_append_fmt (CapeStream, const char*, ...);

__CAPE_LIBEX void            cape_stream_append_c (CapeStream, char);

__CAPE_LIBEX void            cape_stream_append_n (CapeStream, number_t);

__CAPE_LIBEX void            cape_stream_append_f (CapeStream, double);

__CAPE_LIBEX void            cape_stream_append_d (CapeStream, const CapeDatetime*);

__CAPE_LIBEX void            cape_stream_append_stream (CapeStream, CapeStream);

//-----------------------------------------------------------------------------

__CAPE_LIBEX void            cape_stream_append_08  (CapeStream, cape_uint8);

__CAPE_LIBEX void            cape_stream_append_16  (CapeStream, cape_uint16, int network_byte_order);

__CAPE_LIBEX void            cape_stream_append_32  (CapeStream, cape_uint32, int network_byte_order);

__CAPE_LIBEX void            cape_stream_append_64  (CapeStream, cape_uint64, int network_byte_order);

__CAPE_LIBEX void            cape_stream_append_bd  (CapeStream, double, int network_byte_order);

//-----------------------------------------------------------------------------

#endif
