#include "cape_file.h"

#define CAPE_FS_FOLDER_SEP   '/'

//-----------------------------------------------------------------------------

CapeString cape_fs_folders_merge (const char* path1, const char* path2)
{
  return cape_str_catenate_c (path1, CAPE_FS_FOLDER_SEP, path2);
}

//-----------------------------------------------------------------------------

