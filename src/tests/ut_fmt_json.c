#include "fmt/cape_json.h"
#include <stdio.h>

int main (int argc, char *argv[])
{
  CapeUdc h1 = cape_udc_new (CAPE_UDC_FLOAT, NULL);
  
  cape_udc_set_f (h1, -61131231.896387216387162381236871263812);
  
  CapeString s1 = cape_json_to_s (h1);
  
  printf ("OUT: %s\n", s1);
  
  return 0;
}
