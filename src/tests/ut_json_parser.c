#include "fmt/cape_json.h"

int main (int argc, char *argv[])
{
  {
    CapeUdc t1 = cape_json_from_buf ("{\"D\":[{\"id\":\"1\",\"name\":\"config\"},{\"id\":\"2\",\"name\":\"test\"}]}", 59, NULL);
    
    if (t1)
    {
      printf ("OK\n");
    }
    else
    {
      printf ("NG\n");
    }
  }
  
  return 0;
}
