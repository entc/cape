#include "cape_args.h"

//-----------------------------------------------------------------------------------------------------------

CapeUdc cape_args_from_args (int argc, char *argv[], const CapeString name)
{
  CapeUdc params = cape_udc_new (CAPE_UDC_NODE, name);
  
  int i;
  
  for (i = 1; i < argc; i++)
  {
    const char* arg = argv[i];

    if (cape_str_begins (arg, "--"))
    {
      
      
    }
    else if (cape_str_begins (arg, "-"))
    {
      
      
    }
    else
    {
      
      
    }
  }
  
  return params;
}

//-----------------------------------------------------------------------------------------------------------


