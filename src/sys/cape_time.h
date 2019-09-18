#ifndef __CAPE_SYS__TIME__H
#define __CAPE_SYS__TIME__H 1

#include "sys/cape_export.h"
#include "sys/cape_err.h"
#include "stc/cape_str.h"

// c includes
#include <time.h>

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

__CAPE_LIBEX   void            cape_datetime_utc          (CapeDatetime*);

__CAPE_LIBEX   void            cape_datetime_local        (CapeDatetime*);

__CAPE_LIBEX   void            cape_datetime_to_local     (CapeDatetime*);

//-----------------------------------------------------------------------------

                               /* generic method -> use the format for transformation */
__CAPE_LIBEX   CapeString      cape_datetime_s__fmt       (const CapeDatetime*, const CapeString format);

                               /* 2019-09-01 12:08:21 */
__CAPE_LIBEX   CapeString      cape_datetime_s__str       (const CapeDatetime*);   // ISO format

                               /* 20190918-12:07:55.992 */
__CAPE_LIBEX   CapeString      cape_datetime_s__log       (const CapeDatetime*);   // LOG format

                               /* Sun, 11 May 2018 17:05:40 GMT */
__CAPE_LIBEX   CapeString      cape_datetime_s__gmt       (const CapeDatetime*);   // GMT format

                               /* 2019_09_01__12_08_21__ */
__CAPE_LIBEX   CapeString      cape_datetime_s__pre       (const CapeDatetime*);   // prefix format

                               /* 20190901T120821Z */
__CAPE_LIBEX   CapeString      cape_datetime_s__ISO8601   (const CapeDatetime*);   // ISO8601

//-----------------------------------------------------------------------------

__CAPE_LIBEX   time_t          cape_datetime_n__unix      (const CapeDatetime*);

//-----------------------------------------------------------------------------

struct CapeStopTimer_s; typedef struct CapeStopTimer_s* CapeStopTimer;

__CAPE_LIBEX   CapeStopTimer   cape_stoptimer_new         ();

__CAPE_LIBEX   void            cape_stoptimer_del         (CapeStopTimer*);

__CAPE_LIBEX   void            cape_stoptimer_start       (CapeStopTimer);

__CAPE_LIBEX   void            cape_stoptimer_stop        (CapeStopTimer);

__CAPE_LIBEX   void            cape_stoptimer_set         (CapeStopTimer, double);

__CAPE_LIBEX   double          cape_stoptimer_get         (CapeStopTimer);

//-----------------------------------------------------------------------------

#endif




