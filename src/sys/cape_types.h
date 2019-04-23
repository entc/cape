#ifndef __CAPE_TYPES__H
#define __CAPE_TYPES__H 1

#if defined __APPLE__

#include <malloc/malloc.h>
#include <stdlib.h>
#include <memory.h>

#elif defined __OpenBSD__

#include <memory.h>
#include <stdlib.h>

#else

#include <malloc.h>
#include <memory.h>

#endif

//-----------------------------------------------------------------------------

#define u_t unsigned
#define number_t long

//-----------------------------------------------------------------------------

static void* cape_alloc (number_t size)
{
  void* ptr = malloc (size);
  
  memset (ptr, 0, size);

  return ptr;
}

//-----------------------------------------------------------------------------

static void cape_free (void* ptr)
{
  free (ptr);
}

//-----------------------------------------------------------------------------

#define CAPE_ALLOC(size) cape_alloc(size)
#define CAPE_FREE(ptr) cape_free(ptr)

//-----------------------------------------------------------------------------

#define CAPE_NEW( type ) (type*)malloc(sizeof(type))
#define CAPE_DEL( ptr, type ) { memset(*ptr, 0, sizeof(type)); free(*ptr); *ptr = 0; }

//-----------------------------------------------------------------------------

#define CAPE_DIRECTION_FORW 0x0001
#define CAPE_DIRECTION_PREV 0x0002

#endif
