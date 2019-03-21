#include "cape_dl.h"

#include "stc/cape_str.h"
#include "sys/cape_file.h"

#ifdef __linux__
//-----------------------------------------------------------------------------

#include <dlfcn.h>

//-----------------------------------------------------------------------------

struct CapeDl_s
{
  void* handle;
};

//-----------------------------------------------------------------------------

CapeDl cape_dl_new (void)
{
  CapeDl self = CAPE_NEW(struct CapeDl_s);

  self->handle = NULL;
  
  return self;  
}

//-----------------------------------------------------------------------------

void cape_dl_del (CapeDl* p_self)
{
  CapeDl self = *p_self;

  if (self->handle)
  {
    printf("UNLOAD LIBRARY\n");
    
    // this somehow happen automatically
    dlclose (self->handle);
  }

  CAPE_DEL(p_self, struct CapeDl_s);  
}

//-----------------------------------------------------------------------------

int cape_dl_load (CapeDl self, const char* path, const char* name, CapeErr err)
{
  int res;

#ifdef __linux
  CapeString fullname = cape_str_catenate_3 ("lib", name, ".so");
#endif
  CapeString filename = NULL;
  
  if (path)
  {
    filename = cape_fs_folders_merge (path, fullname);
  }
  else
  {
    filename = cape_str_cp (fullname);
  }

  // try to aquire a handle loading the shared library
  self->handle = dlopen (filename, RTLD_NOW | RTLD_NODELETE);
  if (self->handle == NULL)
  {
    res = cape_err_set (err, CAPE_ERR_OS, dlerror());
    goto exit;
  }
  
  res = CAPE_ERR_NONE;

exit:

  cape_str_del (&fullname);
  cape_str_del (&filename);
  
  return res;
}

//-----------------------------------------------------------------------------

void* cape_dl_funct (CapeDl self, const char* name, CapeErr err)
{
  void* function_ptr;
  const char* dlerror_text;
  
  // clear error code  
  dlerror();
  
  function_ptr = dlsym (self->handle, name);
  
  dlerror_text = dlerror ();

  // check if an error ocours
  if (dlerror_text)
  {
    printf ("can't find '%s' function\n", name);
    
    cape_err_set (err, CAPE_ERR_OS, dlerror_text);
    return NULL;
  }
  
  return function_ptr;
}

//-----------------------------------------------------------------------------
#endif
