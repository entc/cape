#include "cape_log.h"

static const char* msg_matrix[7] = { "___", "FAT", "ERR", "WRN", "INF", "DBG", "TRA" };

#if defined _WIN64 || defined _WIN32

#include <windows.h>

  static WORD clr_matrix[11] = {
  0, 
  FOREGROUND_RED | FOREGROUND_INTENSITY,
  FOREGROUND_RED | FOREGROUND_INTENSITY,
  FOREGROUND_RED | FOREGROUND_BLUE | FOREGROUND_INTENSITY,
  FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_RED | FOREGROUND_INTENSITY,
  FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_RED,
  FOREGROUND_GREEN | FOREGROUND_BLUE
  };

#else

#include <sys/time.h>
#include <stdarg.h>
#include <stdio.h>
  
  static const char* clr_matrix[7] = { "0", "0;31", "1;31", "1;33", "0;32", "0;34", "0;30" };


#endif

//-----------------------------------------------------------------------------

void cape_log_msg (CapeLogLevel lvl, const char* unit, const char* method, const char* msg)
{
  char buffer [2050];
  
  if (lvl > 6)
  {
    return;
  }
  
#if defined _WIN64 || defined _WIN32 
  
  _snprintf_s (buffer, 2048, _TRUNCATE, "%-12s %s|%-8s] %s", method, msg_matrix[lvl], unit, msg);
  {
    CONSOLE_SCREEN_BUFFER_INFO info;
    // get the console handle
    HANDLE hStdout = GetStdHandle (STD_OUTPUT_HANDLE);      
    // remember the original background color
    GetConsoleScreenBufferInfo (hStdout, &info);
    // do some fancy stuff
    SetConsoleTextAttribute (hStdout, clr_matrix[lvl]);
    
    printf("%s\n", buffer);
    
    SetConsoleTextAttribute (hStdout, info.wAttributes);
  }

#else
  
  if (msg)
  {
    snprintf (buffer, 2048, "%-12s %s|%-8s] %s", method, msg_matrix[lvl], unit, msg);
  }
  else
  {
    snprintf (buffer, 2048, "%-12s %s|%-8s]", method, msg_matrix[lvl], unit);
  }
  
  printf("\033[%sm%s\033[0m\n", clr_matrix[lvl], buffer);

#endif 
}

//-----------------------------------------------------------------------------

void cape_log_fmt (CapeLogLevel lvl, const char* unit, const char* method, const char* format, ...)
{
  char buffer [1002];
  
  va_list ptr;
  
  va_start(ptr, format);
  
#ifdef _WIN32
  vsnprintf_s (buffer, 1001, 1000, format, ptr);
#else
  vsnprintf (buffer, 1000, format, ptr);
#endif 
  
  cape_log_msg (lvl, unit, method, buffer);
  
  va_end(ptr);
}

//-----------------------------------------------------------------------------

