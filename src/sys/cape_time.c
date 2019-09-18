#include "cape_time.h"

#include "sys/cape_types.h"

//-----------------------------------------------------------------------------

#if defined __LINUX_OS || defined __BSD_OS

#define _BSD_SOURCE

#include <time.h>
#include <sys/time.h>

//-----------------------------------------------------------------------------

void cape_datetime__convert_timeinfo (CapeDatetime* dt, const struct tm* timeinfo)
{
  // fill the timeinfo
  dt->sec = timeinfo->tm_sec;
  dt->msec = 0;
  dt->usec = 0;
  
  dt->minute = timeinfo->tm_min;
  dt->hour = timeinfo->tm_hour;
  
  dt->is_dst = timeinfo->tm_isdst;
  
  dt->day = timeinfo->tm_mday;
  dt->month = timeinfo->tm_mon + 1;
  dt->year = timeinfo->tm_year + 1900; 
}

//-----------------------------------------------------------------------------

void cape_datetime__convert_cape (struct tm* timeinfo, const CapeDatetime* dt)
{
  // fill the timeinfo
  timeinfo->tm_sec   = dt->sec;
  timeinfo->tm_hour  = dt->hour;
  timeinfo->tm_min   = dt->minute;
  
  timeinfo->tm_mday  = dt->day;
  timeinfo->tm_mon   = dt->month - 1;
  timeinfo->tm_year  = dt->year - 1900;  
  
  timeinfo->tm_isdst = dt->is_dst;
}

//-----------------------------------------------------------------------------

void cape_datetime_utc (CapeDatetime* dt)
{
  struct timeval time;  
  struct tm* l01;
  
  gettimeofday (&time, NULL);
  l01 = gmtime (&(time.tv_sec));
  
  cape_datetime__convert_timeinfo (dt, l01); 
  
  dt->msec = time.tv_usec / 1000;
  dt->usec = time.tv_usec;
  
  dt->is_utc = TRUE;
}

//-----------------------------------------------------------------------------

void cape_datetime_local (CapeDatetime* dt)
{
  struct timeval time;  
  struct tm* l01;
  
  gettimeofday (&time, NULL);
  l01 = localtime (&(time.tv_sec));
  
  cape_datetime__convert_timeinfo (dt, l01);
  
  dt->msec = time.tv_usec / 1000;
  dt->usec = time.tv_usec;

  dt->is_utc = FALSE;
}

//-----------------------------------------------------------------------------

void cape_datetime_to_local (CapeDatetime* dt)
{
  if (dt->is_utc)
  {
    struct tm timeinfo;
    struct tm* l01;
    time_t t_of_day;
    unsigned int msec;
    
    cape_datetime__convert_cape (&timeinfo, dt);
    
    // turn on automated DST, depends on the timezone
    timeinfo.tm_isdst = -1;
    
    // the function takes a local time and calculate extra values
    // in our case it is UTC, so we will get the (localtime) UTC value
    // because the function also applies timezone conversion
    t_of_day = mktime (&timeinfo);
    
    // correct (localtime) UTC to (current) UTC :-)
    t_of_day = t_of_day + timeinfo.tm_gmtoff;
    
    // now get the localtime from our UTC
    l01 = localtime (&t_of_day);
    
    // save the msec
    msec = dt->msec;
    
    cape_datetime__convert_timeinfo (dt, l01);
    
    dt->msec = msec;
    
    dt->is_utc = FALSE;
  }
}

//-----------------------------------------------------------------------------

#elif defined __WINDOWS_OS

//-----------------------------------------------------------------------------

#include <windows.h>

//-----------------------------------------------------------------------------

void cape_datetime_utc (CapeDatetime* dt)
{
  SYSTEMTIME time;
  GetSystemTime(&time);
  
  dt->sec = time.wSecond;
  dt->msec = time.wMilliseconds;
  dt->usec = 0;
  
  dt->minute = time.wMinute;
  dt->hour = time.wHour;
  
  dt->day = time.wDay;
  dt->month = time.wMonth;
  dt->year = time.wYear;
  
  dt->is_dst = FALSE;
  dt->is_utc = TRUE;
}

//-----------------------------------------------------------------------------

void cape_datetime_local (CapeDatetime* dt)
{
  SYSTEMTIME time;
  GetLocalTime(&time);
  
  dt->sec = time.wSecond;
  dt->msec = time.wMilliseconds;
  dt->usec = 0;
  
  dt->minute = time.wMinute;
  dt->hour = time.wHour;
  
  dt->day = time.wDay;
  dt->month = time.wMonth;
  dt->year = time.wYear; 
  
  dt->is_dst = FALSE;
  dt->is_utc = FALSE;
}

//-----------------------------------------------------------------------------

void cape_datetime_to_local (CapeDatetime* dt)
{
  // TODO
}

//-----------------------------------------------------------------------------

#endif

//-----------------------------------------------------------------------------

struct CapeStopTimer_s
{
  double time_passed;
  
  struct timeval time_start;  
};

//-----------------------------------------------------------------------------

CapeStopTimer cape_stoptimer_new ()
{
  CapeStopTimer self = CAPE_NEW (struct CapeStopTimer_s);

  self->time_passed = .0;

  memset (&(self->time_start), 0, sizeof(struct timeval));
  
  return self;  
}

//-----------------------------------------------------------------------------

void cape_stoptimer_del (CapeStopTimer* p_self)
{
  if (*p_self)
  {
    
    
    CAPE_DEL (p_self, struct CapeStopTimer_s);
  }
}

//-----------------------------------------------------------------------------

void cape_stoptimer_start (CapeStopTimer self)
{
  gettimeofday (&(self->time_start), NULL);
}

//-----------------------------------------------------------------------------

void cape_stoptimer_stop (CapeStopTimer self)
{
  struct timeval time_res;
  struct timeval time_end;
  
  gettimeofday (&time_end, NULL);
  
  timersub (&time_end, &(self->time_start), &time_res);
  
  self->time_passed += ((double)time_res.tv_sec * 1000) + ((double)time_res.tv_usec / 1000);
}

//-----------------------------------------------------------------------------

void cape_stoptimer_set (CapeStopTimer self, double val_in_ms)
{
  self->time_passed = val_in_ms;
}

//-----------------------------------------------------------------------------

double cape_stoptimer_get (CapeStopTimer self)
{
  return self->time_passed;
}

//-----------------------------------------------------------------------------
