// c includes
#ifdef __GNUC__
#define _GNU_SOURCE 1
#endif

#include "cape_str.h"

// cape includes
#include "sys/cape_err.h"
#include "sys/cape_log.h"

#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <ctype.h>

#include <stdio.h>

//-----------------------------------------------------------------------------

CapeString cape_str_cp (const CapeString source)
{
  if (source == NULL)
  {
    return NULL;
  }  

#ifdef _WIN32
  return _strdup (source);  
#else
  return strdup (source);
#endif    
}

//-----------------------------------------------------------------------------

CapeString cape_str_sub (const CapeString source, number_t len)
{
  char* ret;
  
  // check if we have at least value 
  if (source == NULL)
  {
    return NULL;
  }  
  
  ret = (char*)CAPE_ALLOC( (2 + len) * sizeof(char) );
  
  /* copy the part */
#ifdef _WIN32
  memcpy(ret, source, len * sizeof(char));
#else
  strncpy(ret, source, len);
#endif
  /* set the termination */
  ret[len] = 0;
  
  return ret; 
}

//-----------------------------------------------------------------------------

void cape_str_del (CapeString* p_self)
{
  CapeString self = *p_self;
    
  if(self)
  {
    memset (self, 0x00, strlen(self));
    free (self);
  }
  
  *p_self = NULL;
}

//-----------------------------------------------------------------------------

number_t cape_str_character__len (unsigned char c)
{
  if (0x20 <= c && c <= 0x7E)
  {
    // ascii
    return 1;
  }
  else if ((c & 0xE0) == 0xC0)
  {
    // +1
    return 2;
  }
  else if ((c & 0xF0) == 0xE0)
  {
    // +2
    return 3;
  }
  else if ((c & 0xF8) == 0xF0)
  {
    // +3
    return 4;
  }
  else if ((c & 0xFC) == 0xF8)
  {
    // +4
    return 5;
  }
  else if ((c & 0xFE) == 0xFC)
  {
    // +5
    return 6;
  }
  else
  {
    // not supported character
    return 1;
  }
}

//-----------------------------------------------------------------------------

number_t cape_str_len (const CapeString s)
{
  number_t len = 0;
  const char* s_pos = s;

  while (*s_pos)
  {
    len++;
    s_pos += cape_str_character__len (*s_pos);
  }
  
  return len;
}

//-----------------------------------------------------------------------------

number_t cape_str_size (const CapeString s)
{
  return strlen (s);
}

//-----------------------------------------------------------------------------

int cape_str_empty (const CapeString s)
{
  if (s)
  {
    return (*s == '\0');
  }
  
  return TRUE;
}

//-----------------------------------------------------------------------------

int cape_str_not_empty (const CapeString s)
{
  if (s)
  {
    return (*s != '\0');
  }
  
  return FALSE;
}

//-----------------------------------------------------------------------------

int cape_str_equal (const CapeString s1, const CapeString s2)
{
  if ((NULL == s1) || (NULL == s2))
  {
    return FALSE;
  }

  return strcmp (s1, s2) == 0;
}

//-----------------------------------------------------------------------------

int cape_str_compare (const CapeString s1, const CapeString s2)
{
  if ((NULL == s1) || (NULL == s2))
  {
    return FALSE;
  }
  
#ifdef _MSC_VER
  
  return _stricmp (s1, s2) == 0;
  
#elif __GNUC__  

  return strcasecmp (s1, s2) == 0;

#endif
}

//-----------------------------------------------------------------------------

int cape_str_begins (const CapeString s, const CapeString begins_with)
{
  if (s == NULL)
  {
    return FALSE;
  }
  
  if (begins_with == NULL)
  {
    return FALSE;
  }
  
  {
    int len = strlen(begins_with);
    
    return strncmp (s, begins_with, len) == 0;
  }
}

//-----------------------------------------------------------------------------

CapeString cape_str_uuid (void)
{
  CapeString self = CAPE_ALLOC(38);
  
  sprintf(self, "%04X%04X-%04X-%04X-%04X-%04X%04X%04X", 
          rand() & 0xffff, rand() & 0xffff,                          // Generates a 64-bit Hex number
          rand() & 0xffff,                                           // Generates a 32-bit Hex number
          ((rand() & 0x0fff) | 0x4000),                              // Generates a 32-bit Hex number of the form 4xxx (4 indicates the UUID version)
          rand() % 0x3fff + 0x8000,                                  // Generates a 32-bit Hex number in the range [0x8000, 0xbfff]
          rand() & 0xffff, rand() & 0xffff, rand() & 0xffff);        // Generates a 96-bit Hex number
  
  return self;
}

//-----------------------------------------------------------------------------

CapeString cape_str_flp (const CapeString format, va_list valist)
{
  CapeString ret = NULL;
  
  #ifdef _MSC_VER
  
  {
    int len = _vscprintf (format, valist) + 1;
    
    ret = CAPE_ALLOC (len);
    
    len = vsprintf_s (ret, len, format, valist);
  }
  
  #elif __GNUC__
  
  {
    char* strp;
    
    int bytesWritten = vasprintf (&strp, format, valist);
    
    if (bytesWritten == -1)
    {
      CapeErr err = cape_err_new ();
      
      cape_err_lastOSError (err);
      
      cape_log_fmt (CAPE_LL_ERROR, "CAPE", "str_fmt", "can't format string: %s", cape_err_text(err));
      
      cape_err_del (&err);
    }
    else if (bytesWritten == 0)
    {
      cape_log_fmt (CAPE_LL_WARN, "CAPE", "str_fmt", "format of string returned 0");      
    }
    else if (bytesWritten > 0)
    {
      ret = strp;
    }
  }
  
  #elif __BORLANDC__
  
  {
    int len = 1024;
    
    ret = CAPE_NEW (len);
    
    len = vsnprintf (ret, len, format, valist);    
  }
  
  #endif
  
  return ret;
}

//-----------------------------------------------------------------------------

CapeString cape_str_fmt (const CapeString format, ...)
{
  CapeString ret = NULL;
  
  va_list ptr;  
  va_start(ptr, format);
  
  ret = cape_str_flp (format, ptr);
  
  va_end(ptr); 
  
  return ret;  
}

//-----------------------------------------------------------------------------

CapeString cape_str_catenate_2 (const CapeString s1, const CapeString s2)
{
  if( !s1 )
  {
    return cape_str_cp (s2);  
  }
  
  if( !s2 )
  {
    return cape_str_cp (s1);  
  }
  
  {
    /* variables */
    int s1_len = strlen(s1);
    int s2_len = strlen(s2);
    
    char* ret;
    char* pos;
    
    ret = (char*)CAPE_ALLOC( (s1_len + s2_len + 10) );
    if (ret == NULL) 
    {
      return NULL;
    }
    
    pos = ret;
    
    memcpy(pos, s1, s1_len);
    pos = pos + s1_len;
    
    memcpy(pos, s2, s2_len);
    pos = pos + s2_len;
    
    *pos = 0;
    
    return ret;
  }
}

//-----------------------------------------------------------------------------

CapeString cape_str_catenate_3 (const CapeString s1, const CapeString s2, const CapeString s3)
{
  number_t s1_len = strlen(s1);
  number_t s2_len = strlen(s2);
  number_t s3_len = strlen(s3);
  
  char* ret = (char*)CAPE_ALLOC( (s1_len + s2_len + s3_len + 1) * sizeof(char) );
  
  char* pos = ret;
  
  memcpy(pos, s1, s1_len);
  pos = pos + s1_len;
  
  memcpy(pos, s2, s2_len);
  pos = pos + s2_len;
  
  memcpy(pos, s3, s3_len);
  pos = pos + s3_len;
  
  *pos = 0;
  
  return ret;   
}

//-----------------------------------------------------------------------------

CapeString cape_str_catenate_c (const CapeString s1, char c, const CapeString s2)
{
  // variables
  number_t s1_len;
  number_t s2_len;
  char* ret;
  char* pos;
  
  if (s1 == NULL) 
  {
    return NULL;
  }
  
  if (s2 == NULL) 
  {
    return NULL;
  }
  
  s1_len = strlen(s1);
  s2_len = strlen(s2);
  
  ret = (char*)CAPE_ALLOC( (s1_len + s2_len + 2) );
  
  if (ret == NULL) 
  {
    return NULL;
  }
  
  pos = ret;
  
  if (s1_len > 0) 
  {
    memcpy(ret, s1, s1_len);
    pos = pos + s1_len;
  }
  
  *pos = c;
  pos++;
  
  if (s2_len > 0)
  {
    memcpy(pos, s2, s2_len);
    pos = pos + s2_len;    
  }
  
  *pos = 0;
  
  return ret;  
}

//-----------------------------------------------------------------------------

void cape_str_replace_cp (CapeString* p_self, const CapeString source)
{
  // create a new copy
  CapeString self = cape_str_cp (source);

  // free the old string  
  cape_str_del (p_self);
  
  // transfer ownership
  *p_self = self;
}

//-----------------------------------------------------------------------------

void cape_str_replace_mv (CapeString* p_self, CapeString* p_source)
{
  // free the old string  
  cape_str_del (p_self);

  // transfer ownership
  *p_self = *p_source;

  // release ownership
  *p_source = NULL;
}

//-----------------------------------------------------------------------------

void cape_str_fill (CapeString self, number_t len, const CapeString source)
{
  // assume that the source will fit into the self object
  strncpy (self, source, len);
  
  self[len] = '\0';
}

//-----------------------------------------------------------------------------

void cape_str_to_upper (CapeString self)
{
  char* pos = self;
  
  while (*pos)
  {
    *pos = toupper (*pos);
    pos++;
  }
}

//-----------------------------------------------------------------------------

void cape_str_to_lower (CapeString self)
{
  char* pos = self;
  
  while (*pos)
  {
    *pos = tolower (*pos);
    pos++;
  }
}

//-----------------------------------------------------------------------------

