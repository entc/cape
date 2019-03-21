#ifndef __CAPE_FMT__JSON__H
#define __CAPE_FMT__JSON__H 1

#include "sys/cape_export.h"
#include "sys/cape_types.h"
#include "stc/cape_udc.h"

//-----------------------------------------------------------------------------

__CAPE_LIBEX   CapeUdc           cape_json_from_s           (const CapeString, const CapeString name);

__CAPE_LIBEX   CapeUdc           cape_json_from_buf         (const char* buffer, number_t size, const CapeString name);

__CAPE_LIBEX   CapeString        cape_json_to_s             (const CapeUdc source);

//-----------------------------------------------------------------------------

#endif

