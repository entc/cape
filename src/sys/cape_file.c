#include "cape_file.h"
#include "cape_log.h"

#include "stc/cape_list.h"

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
#include <dirent.h>

#define CAPE_FS_FOLDER_SEP   '/'

#elif defined __BSD_OS

#include <unistd.h>
#include <limits.h>
#include <errno.h>
#include <stdio.h>
#include <sys/stat.h>
#include <dirent.h>

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

CapeString cape_fs_path_current (const char* filepath)
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
  
#elif defined __LINUX_OS || defined __BSD_OS

  char* ret = CAPE_ALLOC (PATH_MAX + 1);
  
  getcwd (ret, PATH_MAX);
  
#endif

  if (filepath)
  {
    CapeString h = cape_fs_path_merge (ret, filepath);
    
    CAPE_FREE(ret);
    
    return h;
  }
  else
  {
    return ret;
  }  
}

//-----------------------------------------------------------------------------

CapeString cape_fs_path_absolute (const char* filepath)
{
  if (filepath == NULL)
  {
    return cape_fs_path_current (NULL);
  }
  
  if (cape_fs_is_relative (filepath))
  {
    return cape_fs_path_current (filepath);
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
    return cape_fs_path_current (NULL);
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
      *p_path = cape_fs_path_current (NULL);
    }
  }

  return ret;
}

//-----------------------------------------------------------------------------

int cape_fs_path_create (const char* path, CapeErr err)
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

static void __STDCALL cape_fs_path_size__on_del (void* ptr)
{
  CapeString h = ptr; cape_str_del (&h);
}

//-----------------------------------------------------------------------------

number_t cape_fs_path_size__process_path (DIR* dir, CapeList folders, const char* path, CapeErr err)
{
  number_t total_size = 0;
  struct dirent* dentry;
  
  for (dentry = readdir (dir); dentry; dentry = readdir (dir))
  {
    if (dentry->d_type == DT_DIR)
    {
      // excluse special folders
      if (cape_str_equal (dentry->d_name, ".") || cape_str_equal (dentry->d_name, ".."))
      {
        continue;
      }
      
      // construct new path and adds to the folders list
      {
        CapeString h = cape_fs_path_merge (path, dentry->d_name);

        cape_list_push_back (folders, h);
      }
    }
    else if (dentry->d_type == DT_REG)
    {
      CapeString file = cape_fs_path_merge (path, dentry->d_name);
      
      struct stat st;
      if (stat (file, &st) != 0)
      {
        cape_str_del (&file);
        
        cape_err_lastOSError (err);
        return 0;
      }
      else
      {
        total_size += st.st_size;
        
        cape_str_del (&file);
      }
    }
  }
  
  return total_size;
}

//-----------------------------------------------------------------------------

number_t cape_fs_path_size (const char* path, CapeErr err)
{
  number_t total_size = 0;
  CapeList folders = cape_list_new (cape_fs_path_size__on_del);

  {
    DIR* dir = opendir (path);
    
    if (dir == NULL)
    {
      cape_err_lastOSError (err);
      goto exit_and_cleanup;
    }
    
    total_size += cape_fs_path_size__process_path (dir, folders, path, err);
    
    closedir (dir);
  }
  
  if (cape_err_code (err))
  {
    total_size = 0;
    goto exit_and_cleanup;
  }
  
  // iterate through all
  {
    CapeListCursor cursor; cape_list_cursor_init (folders, &cursor, CAPE_DIRECTION_FORW);
    
    while (cape_list_cursor_next(&cursor))
    {
      CapeString sub_path = cape_list_node_data (cursor.node);
      
      total_size += cape_fs_path_size (sub_path, err);
      
      if (cape_err_code (err))
      {
        total_size = 0;
        goto exit_and_cleanup;
      }
    }
  }
  
exit_and_cleanup:
  
  cape_list_del (&folders);
  return total_size;
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

number_t cape_fh_write_buf (CapeFileHandle self, const char* bufdat, number_t buflen)
{
  return write (self->fd, bufdat, buflen);
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
