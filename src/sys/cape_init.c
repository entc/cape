#include <time.h>
#include <stdlib.h>

//-----------------------------------------------------------------------------

__attribute__((constructor)) void library_constructor ()
{
  // initialize random generator
  srand (time (NULL));
}

//-----------------------------------------------------------------------------

__attribute__((destructor)) void library_destructor ()
{
  
}

//-----------------------------------------------------------------------------
