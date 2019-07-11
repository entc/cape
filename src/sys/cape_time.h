#ifndef __CAPE_SYS__TIME__H
#define __CAPE_SYS__TIME__H 1

#include "sys/cape_export.h"
#include "sys/cape_err.h"

//=============================================================================

#pragma pack(push, 16)
typedef struct
{  
  unsigned int day;
  unsigned int month;
  unsigned int year;
  
  unsigned int hour;
  unsigned int minute;
  
  unsigned int usec;
  unsigned int msec;
  unsigned int sec;
  
  int is_dst;
  int is_utc;
  
} CapeDatetime;
#pragma pack(pop)

//-----------------------------------------------------------------------------

__CAPE_LIBEX void   cape_datetime_utc        (CapeDatetime*);

__CAPE_LIBEX void   cape_datetime_local      (CapeDatetime*);

__CAPE_LIBEX void   cape_datetime_to_local   (CapeDatetime*);

//-----------------------------------------------------------------------------

#endif




