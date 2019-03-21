#include "cape_tokenizer.h"


//-----------------------------------------------------------------------------

static void __STDCALL cape_tokenizer_onDestroy (void* ptr)
{
  CapeString h = ptr; cape_str_del (&h);
}

//-----------------------------------------------------------------------------------------------------------

CapeList cape_tokenizer_buf (const char* buffer, number_t len, char delimeter)
{
  CapeList tokens = cape_list_new (cape_tokenizer_onDestroy);
  
  const char* posR = buffer;
  const char* posL = buffer;
  
  number_t posI = 0;
  
  if (buffer == NULL)
  {
    return tokens;
  }
  
  for (posI = 0; posI < len; posI++)
  {
    if (*posR == delimeter)
    {
      // calculate the length of the last segment
      CapeString h = cape_str_sub (posL, posR - posL);

      cape_list_push_back (tokens, h);
      
      posL = posR + 1;
    }
    
    posR++;
  }
  
  {
    CapeString h = cape_str_sub (posL, posR - posL);
    
    cape_list_push_back (tokens, h);
  }
  
  return tokens;  
}

//-----------------------------------------------------------------------------------------------------------

int cape_tokenizer_split (const CapeString source, char token, CapeString* p_left, CapeString* p_right)
{
  if (source == NULL)
  {
    return FALSE;
  }
  {
    const char * pos = strchr (source, token);
    if (pos != NULL)
    {
      CapeString h = cape_str_sub (source, pos - source);
      
      cape_str_replace_mv (p_left, &h);
      cape_str_replace_cp (p_right, pos + 1);
      
      return TRUE;
    }
    
    return FALSE;
  }
}

//-----------------------------------------------------------------------------------------------------------
