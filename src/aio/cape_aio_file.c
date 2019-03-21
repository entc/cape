#include "cape_aio_file.h"

#include <malloc.h>

//-----------------------------------------------------------------------------

struct CapeAioFileReader_s
{
  
    void* handle;
    
    void* hobj;
    
    
};

//-----------------------------------------------------------------------------

CapeAioFileReader cape_aio_freader_new (void* handle)
{
  CapeAioFileReader self = malloc(sizeof(struct CapeAioFileReader_s));
  
  self->handle = handle;
  
  return self;
}

//-----------------------------------------------------------------------------

static int __STDCALL cape_aio_freader_onEvent (void* ptr, void* handle, int hflags, unsigned long events, void* overlapped, unsigned long param1)
{
    printf ("FILE READER READY\n");
}

//-----------------------------------------------------------------------------

static void __STDCALL cape_aio_freader_onDestroy (void* ptr, CapeAioHandle aioh)
{
    
}

//-----------------------------------------------------------------------------

int cape_aio_freader_add (CapeAioFileReader* p_self, CapeAioContext aio)
{
  CapeAioFileReader self = *p_self;
  
  *p_self = NULL;
    
  self->hobj = cape_aio_handle_new (self->handle, CAPE_AIO_READ, self, cape_aio_freader_onEvent, cape_aio_freader_onDestroy);
  
  cape_aio_context_add (aio, self->hobj);      
  
  return 0;
}

//-----------------------------------------------------------------------------
