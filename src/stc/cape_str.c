#include "cape_str.h"

#include <malloc.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <ctype.h>

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

int cape_str_equal (const CapeString s1, const CapeString s2)
{
  if (NULL == s1)
  {
    return FALSE;
  }

  if (NULL == s2)
  {
    return FALSE;
  }
  
  return strcmp (s1, s2) == 0;
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

