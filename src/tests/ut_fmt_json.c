#include "fmt/cape_json.h"
#include <stdio.h>

int main (int argc, char *argv[])
{
  {
    CapeUdc t1 = cape_json_from_buf ("{\"D\":[{\"id\":\"1\",\"name\":\"config\"},{\"id\":\"2\",\"name\":\"test\"}]}", 59);
    
    if (t1)
    {
      printf ("OK\n");
    }
    else
    {
      printf ("NG\n");
    }
  }
  
  {
    CapeUdc h1;
    CapeUdc h2;
    CapeString s1;
    CapeString s2;
    
    h1 = cape_udc_new (CAPE_UDC_NODE, NULL);

    cape_udc_add_n (h1, "val1", -2);
    cape_udc_add_n (h1, "val2", 0);
    cape_udc_add_n (h1, "val3", 2);
    
    s1 = cape_json_to_s (h1);
    
    printf ("S1: %s\n", s1);
    
    h2 = cape_json_from_s (s1);
    
    s2 = cape_json_to_s (h1);
    
    printf ("S2: %s\n", s2);
  }
  
  return 0;
}
