#include "cape_stream.h"

// c includes
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <memory.h>
#include <limits.h>
#include <stdlib.h>

//-----------------------------------------------------------------------------

struct CapeStream_s
{
  number_t size;
  
  char* buffer;
  
  char* pos;
  
};

//-----------------------------------------------------------------------------

number_t cape_stream_size (CapeStream self)
{
  return self->pos - self->buffer;
}

//-----------------------------------------------------------------------------

void cape_stream_allocate (CapeStream self, unsigned long amount)
{
  // safe how much we have used from the buffer
  unsigned long usedBytes = cape_stream_size (self);
  
  // use realloc to minimalize coping the buffer
  self->size += amount;
  self->buffer = realloc(self->buffer, self->size + 1);
  
  // reset the position
  self->pos = self->buffer + usedBytes;
}

//-----------------------------------------------------------------------------

void cape_stream_reserve (CapeStream self, unsigned long amount)
{
  unsigned long diffBytes = cape_stream_size (self) + amount + 1;
  
  if (diffBytes > self->size)
  {
    if (amount > self->size)
    {
      cape_stream_allocate (self, amount);
    }
    else
    {
      cape_stream_allocate (self, self->size + self->size);
    }
  }
}

//-----------------------------------------------------------------------------

CapeStream cape_stream_new ()
{
  CapeStream self = CAPE_NEW(struct CapeStream_s);
  
  self->size = 0;
  self->buffer = 0;
  self->pos = self->buffer;
  
  // initial alloc
  cape_stream_allocate (self, 100);
  
  // clean
  cape_stream_clr (self);
  
  return self;
}

//-----------------------------------------------------------------------------

void cape_stream_del (CapeStream* pself)
{
  CapeStream self = *pself;
  
  if (self)
  {
    free (self->buffer);
    
    CAPE_DEL (pself, struct CapeStream_s);
  }  
}

//-----------------------------------------------------------------------------

CapeString cape_stream_to_str (CapeStream* pself)
{
  CapeStream self = *pself;  
  CapeString ret = self->buffer;
  
  // set terminator
  *(self->pos) = 0;
  
  CAPE_DEL(pself, struct CapeStream_s);
  
  return ret;
}

//-----------------------------------------------------------------------------

number_t cape_stream_to_n (CapeStream self)
{
  // prepare and add termination
  *(self->pos) = '\0';
  
  number_t ret = strtol (self->buffer, NULL, 10);
    
  cape_stream_clr (self);
  
  return ret;
}

//-----------------------------------------------------------------------------

CapeString cape_stream_to_s (CapeStream self)
{
  CapeString ret = cape_str_sub (self->buffer, self->pos - self->buffer);
  
  cape_stream_clr (self);
  
  return ret;
}

//-----------------------------------------------------------------------------

void cape_stream_clr (CapeStream self)
{
  self->pos = self->buffer;
}

//-----------------------------------------------------------------------------

const char* cape_stream_get (CapeStream self)
{
  // set terminator
  *(self->pos) = 0;
  
  return self->buffer;
}

//-----------------------------------------------------------------------------

const char* cape_stream_data (CapeStream self)
{
  return self->buffer;
}

//-----------------------------------------------------------------------------

void cape_stream_append_str (CapeStream self, const char* s)
{
  if (s)
  {
    // need to find the length
    cape_stream_append_buf (self, s, strlen(s));
  }
}

//-----------------------------------------------------------------------------

void cape_stream_append_buf (CapeStream self, const char* buffer, unsigned long size)
{
  if (size > 0)
  {
    cape_stream_reserve (self, size);
    
    memcpy (self->pos, buffer, size);
    self->pos += size;
  }
}

//-----------------------------------------------------------------------------

void cape_stream_append_fmt (CapeStream self, const char* format, ...)
{
  // variables
  va_list valist;
  va_start (valist, format);
  
  #ifdef _MSC_VER
  
  {
    int len = _vscprintf (format, valist) + 1;
    
    cape_stream_reserve (self, len);
    
    len = vsprintf_s (self->pos, len, format, valist);
    
    self->pos += len;
  }
  
  #elif _GCC
  
  {
    char* strp;
    
    int bytesWritten = vasprintf (&strp, format, valist);
    if ((bytesWritten > 0) && strp)
    {
      cape_stream_append_buf (self, strp, bytesWritten);
      free(strp);
    }
  }
  
  #elif __BORLANDC__
  
  {
    int len = 1024;
    
    cape_stream_reserve (self, len);
    
    len = vsnprintf (self->pos, len, format, valist);
    
    self->pos += len;
  }
  
  #endif
  
  va_end(valist);
}

//-----------------------------------------------------------------------------

void cape_stream_append_c (CapeStream self, char c)
{
  cape_stream_reserve (self, 1);
  
  *(self->pos) = c;
  self->pos++;
}

//-----------------------------------------------------------------------------

void cape_stream_append_n (CapeStream self, number_t val)
{
  cape_stream_reserve (self, 26);  // for very long intergers
  
#ifdef _MSC_VER
  
  self->pos += _snprintf_s (self->pos, 24, _TRUNCATE, "%lu", val);
  
#else
  
  self->pos += snprintf(self->pos, 24, "%lu", val);
  
#endif
}

//-----------------------------------------------------------------------------

void cape_stream_append_f (CapeStream self, double val)
{
  cape_stream_reserve (self, 26);  // for very long intergers
  
#ifdef _MSC_VER
  
  self->pos += _snprintf_s (self->pos, 24, _TRUNCATE, "%f", val);
  
#else
  
  self->pos += snprintf(self->pos, 24, "%f", val);
  
#endif
}

//-----------------------------------------------------------------------------

void cape_stream_append_stream (CapeStream self, CapeStream stream)
{
  unsigned long usedBytes = stream->pos - stream->buffer;
  
  cape_stream_reserve (self, usedBytes);
  
  memcpy (self->pos, stream->buffer, usedBytes);
  self->pos += usedBytes;
}

//-----------------------------------------------------------------------------
