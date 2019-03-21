#include "cape_mutex.h"
#include "sys/cape_types.h"

#include <pthread.h>

//-----------------------------------------------------------------------------

CapeMutex cape_mutex_new (void)
{
  pthread_mutex_t* self = CAPE_NEW(pthread_mutex_t);
  
  pthread_mutex_init (self, NULL);
  
  return self;
}

//-----------------------------------------------------------------------------

void cape_mutex_del (CapeMutex* p_self)
{
  pthread_mutex_t* self = *p_self;
  
  pthread_mutex_destroy (self);

  CAPE_DEL(p_self, pthread_mutex_t);  
}

//-----------------------------------------------------------------------------

void cape_mutex_lock (CapeMutex self)
{
  pthread_mutex_lock (self);
}

//-----------------------------------------------------------------------------

void cape_mutex_unlock (CapeMutex self)
{
  pthread_mutex_unlock (self);
}

//-----------------------------------------------------------------------------
