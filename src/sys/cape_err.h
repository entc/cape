#ifndef __CAPE_SYS__ERR__H
#define __CAPE_SYS__ERR__H 1

#include "sys/cape_export.h"

//=============================================================================

#define CAPE_ERR_NONE           0x0000
#define CAPE_ERR_OS             0x0001
#define CAPE_ERR_LIB            0x0002
#define CAPE_ERR_3RDPARTY_LIB   0x0004
#define CAPE_ERR_NO_OBJECT      0x0005
#define CAPE_ERR_RUNTIME        0x0006
#define CAPE_ERR_CONTINUE       0x0007
#define CAPE_ERR_PARSER         0x0008
#define CAPE_ERR_NOT_FOUND      0x0009
#define CAPE_ERR_MISSING_PARAM  0x000A

//=============================================================================

struct CapeErr_s; typedef struct CapeErr_s* CapeErr;

//-----------------------------------------------------------------------------

__CAPE_LIBEX   CapeErr           cape_err_new           (void);            // allocate memory and initialize the object

__CAPE_LIBEX   void              cape_err_del           (CapeErr*);         // release memory

//-----------------------------------------------------------------------------

__CAPE_LIBEX   int               cape_err_set           (CapeErr, unsigned long errCode, const char* error_message);

__CAPE_LIBEX   int               cape_err_set_fmt       (CapeErr, unsigned long errCode, const char* error_message, ...);

__CAPE_LIBEX   const char*       cape_err_text          (CapeErr);

__CAPE_LIBEX   unsigned long     cape_err_code          (CapeErr);

__CAPE_LIBEX   int               cape_err_lastOSError   (CapeErr);

__CAPE_LIBEX   int               cape_err_formatErrorOS (CapeErr, unsigned long errCode);

//-----------------------------------------------------------------------------

#endif

