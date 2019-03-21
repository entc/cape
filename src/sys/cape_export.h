#ifndef __CAPE_SYS__EXPORT__H
#define __CAPE_SYS__EXPORT__H 1

#if defined _WIN64 || defined _WIN32

#define __CAPE_LIBEX  __declspec( dllexport )
#define __STDCALL __stdcall

#else

#ifdef __cplusplus
  #define __CAPE_LIBEX extern "C" 
#else
  #define __CAPE_LIBEX
#endif

#define __STDCALL

#endif

#define TRUE 1
#define FALSE 0

#endif
