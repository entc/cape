#include "cape_file.h"
#include "cape_log.h"

//-----------------------------------------------------------------------------

#ifdef __WINDOWS_OS

#include <windows.h>
#include <shlwapi.h>
#define CAPE_FS_FOLDER_SEP   '\\'

#elif defined __LINUX_OS 

#include <unistd.h>
#include <stdlib.h>
#include <linux/limits.h>
#include <errno.h>
#include <sys/stat.h>
#include <stdio.h>

#define CAPE_FS_FOLDER_SEP   '/'

#elif defined __BSD_OS

#include <unistd.h>
#include <limits.h>
#include <errno.h>
#include <stdio.h>
#include <sys/stat.h>

#define CAPE_FS_FOLDER_SEP   '/'

#endif

//-----------------------------------------------------------------------------

CapeString cape_fs_path_merge (const char* path1, const char* path2)
{
  return cape_str_catenate_c (path1, CAPE_FS_FOLDER_SEP, path2);
}

//-----------------------------------------------------------------------------

int cape_fs_is_relative (const char* filepath)
{
  if (filepath == NULL)
  {
    return TRUE;
  }
  
#ifdef __WINDOWS_OS

  // use windows API
  return PathIsRelative (filepath);

#elif defined __LINUX_OS || defined __BSD_OS

  // trivial approach: check if first character is the separator
  return (*filepath != CAPE_FS_FOLDER_SEP);

#endif
}

//-----------------------------------------------------------------------------

CapeString cape_fs_path_current (void)
{
#ifdef __WINDOWS_OS
  
  char* ret = CAPE_ALLOC (MAX_PATH + 1);
  DWORD dwRet;
  
  // use windows API
  dwRet = GetCurrentDirectory (MAX_PATH, ret);

  if ((dwRet == 0) || (dwRet > MAX_PATH))
  {
    CAPE_FREE (ret);
    return NULL;
  }
  
  return ret;
  
#elif defined __LINUX_OS || defined __BSD_OS

  char* ret = CAPE_ALLOC (PATH_MAX + 1);
  
  getcwd (ret, PATH_MAX);
  
  return ret;

#endif
}

//-----------------------------------------------------------------------------

CapeString cape_fs_path_absolute (const char* filepath)
{
  if (filepath == NULL)
  {
    return cape_fs_path_current ();
  }
  
  if (cape_fs_is_relative (filepath))
  {
    CapeString h = cape_fs_path_current ();
    CapeString r = cape_fs_path_merge (h, filepath);
    
    cape_str_del (&h);
    
    return r;
  }
  else
  {
    return cape_str_cp (filepath);
  }
}

//-----------------------------------------------------------------------------

CapeString cape_fs_path_resolve (const char* filepath, CapeErr err)
{
  if (filepath == NULL)
  {
    return cape_fs_path_current ();
  }
  
#ifdef __WINDOWS_OS

  {
    char* ret = CAPE_ALLOC (MAX_PATH + 1);
    
    if (!PathCanonicalize (ret, filepath))
    {
      CAPE_FREE (ret);
      return NULL;
    }
    
    return ret;
  }

#elif defined __LINUX_OS || defined __BSD_OS

  {
    char* ret = CAPE_ALLOC (PATH_MAX + 1);

    if (!realpath (filepath, ret))
    {
      cape_err_lastOSError (err);
      
      cape_log_msg (CAPE_LL_ERROR, "CAPE", "path absolute", cape_err_text(err));
      
      CAPE_FREE (ret);
      return NULL;
    }
    
    return ret;
  }

#endif
}

//-----------------------------------------------------------------------------

const CapeString cape_fs_split (const char* filepath, CapeString* p_path)
{
  const char* last_sep_position = NULL;
  const char* ret = NULL;
  
  if (filepath == NULL)
  {
    return NULL;
  }
  
  // try to find the last position of the separator
  last_sep_position = strrchr (filepath, CAPE_FS_FOLDER_SEP);
  
  if (last_sep_position)
  {
    // we do have a path and file
    ret = last_sep_position + 1;

    if (p_path)
    {
      // extract the path from the origin filepath
      CapeString path = cape_str_sub (filepath, last_sep_position - filepath);

      // correct the path
      CapeString h = cape_fs_path_absolute (path);

      // override path
      cape_str_replace_mv (&path, &h);
      
      *p_path = path;
    }
  }
  else
  {
    // we do have only a file
    ret = filepath;
    
    if (p_path)
    {
      *p_path = cape_fs_path_current ();
    }
  }

  return ret;
}

//-----------------------------------------------------------------------------

int cape_path_create (const char* path, CapeErr err)
{
#ifdef __WINDOWS_OS
  
#elif defined __LINUX_OS || defined __BSD_OS

  int res = mkdir (path, 0770);
  if (res)
  {
    int err_no = errno;
    if (err_no != EEXIST)
    {
      res = cape_err_formatErrorOS (err, err_no);
      
      //cape_log_msg (CAPE_LL_ERROR, "CAPE", "path create", cape_err_text(err));
      
      return res;
    }
  }

  return CAPE_ERR_NONE;
  
#endif  
}

//-----------------------------------------------------------------------------

struct CapeFileHandle_s
{
  CapeString file;
  
  number_t fd;
};

//-----------------------------------------------------------------------------

CapeFileHandle cape_fh_new (const CapeString path, const CapeString file)
{
  CapeFileHandle self = CAPE_NEW (struct CapeFileHandle_s);
  
  if (path)
  {
    self->file = cape_fs_path_merge (path, file);
  }
  else
  {
    self->file = cape_str_cp (file);
  }
  
  self->fd = -1;
  
  return self;
}

//-----------------------------------------------------------------------------

void cape_fh_del (CapeFileHandle* p_self)
{
  if (*p_self)
  {
    CapeFileHandle self = *p_self;
    
    cape_str_del (&(self->file));
    
    if (self->fd >= 0)
    {
      close (self->fd);
    }
    
    CAPE_DEL (p_self, struct CapeFileHandle_s);
  }  
}

//-----------------------------------------------------------------------------

int cape_fh_open (CapeFileHandle self, int flags, CapeErr err)
{
  self->fd = open (self->file, flags, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP);
  
  if (self->fd == -1)
  {
    return cape_err_lastOSError (err);
  }
  
  return CAPE_ERR_NONE;
}

//-----------------------------------------------------------------------------

void* cape_fh_fd (CapeFileHandle self)
{
  return (void*)self->fd;
}

//-----------------------------------------------------------------------------

number_t cape_fh_read_buf (CapeFileHandle self, char* bufdat, number_t buflen)
{
  int res = read (self->fd, bufdat, buflen);
  if (res < 0)
  {
    return 0;
  }
  
  return res;
}

//-----------------------------------------------------------------------------

const void* cape_fs_stdout (void)
{
  return stdout;
}

//-----------------------------------------------------------------------------

void cape_fs_write_msg (const void* handle, const char* buf, number_t len)
{
  if (handle)
  {
    write ((number_t)handle, buf, len);
  }
}

//-----------------------------------------------------------------------------

void cape_fs_writeln_msg (const void* handle, const char* buf, number_t len)
{
  if (handle)
  {
    write ((number_t)handle, buf, len);
    write ((number_t)handle, "\n", 1);
  }
}

//-----------------------------------------------------------------------------

void cape_fs_write_fmt (const void* handle, const char* format, ...)
{
  
}

//-----------------------------------------------------------------------------
