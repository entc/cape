#ifndef __CAPE_SYS__FILE__H
#define __CAPE_SYS__FILE__H 1

#include "sys/cape_export.h"
#include "sys/cape_err.h"
#include "stc/cape_str.h"

#include <fcntl.h>

//=============================================================================

__CAPE_LIBEX   CapeString         cape_fs_path_merge     (const char* path1, const char* path2);

__CAPE_LIBEX   CapeString         cape_fs_path_current   (void);

__CAPE_LIBEX   CapeString         cape_fs_path_absolute  (const char* filepath);

__CAPE_LIBEX   CapeString         cape_fs_path_resolve   (const char* filepath, CapeErr);

//-----------------------------------------------------------------------------

// splits a filepath into path and filename, returns the filename, sets the path if not NULL
// returns always absolute paths
__CAPE_LIBEX   const CapeString   cape_fs_split          (const char* filepath, CapeString* p_path);

//-----------------------------------------------------------------------------

__CAPE_LIBEX   int                cape_path_create       (const char* path, CapeErr);

//=============================================================================

struct CapeFileHandle_s; typedef struct CapeFileHandle_s* CapeFileHandle;

//-----------------------------------------------------------------------------

__CAPE_LIBEX   CapeFileHandle     cape_fh_new     (const CapeString path, const CapeString file);

__CAPE_LIBEX   void               cape_fh_del     (CapeFileHandle*);

__CAPE_LIBEX   int                cape_fh_open    (CapeFileHandle, int flags, CapeErr);

__CAPE_LIBEX   void*              cape_fh_fd      (CapeFileHandle);

//-----------------------------------------------------------------------------

__CAPE_LIBEX   const void*        cape_fs_stdout         (void);

__CAPE_LIBEX   void               cape_fs_write_msg      (const void* handle, const char* buf, number_t len);

__CAPE_LIBEX   void               cape_fs_writeln_msg    (const void* handle, const char* buf, number_t len);

__CAPE_LIBEX   void               cape_fs_write_fmt      (const void* handle, const char* format, ...);

//-----------------------------------------------------------------------------

#endif
