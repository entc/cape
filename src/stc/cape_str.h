#ifndef __CAPE_STC__STR__H
#define __CAPE_STC__STR__H 1

#include "sys/cape_export.h"
#include "sys/cape_types.h"

// included for va_list
#include <stdarg.h>

//=============================================================================

#define CapeString char*

__CAPE_LIBEX   CapeString         cape_str_cp            (const CapeString);                            // allocate memory and initialize the object

__CAPE_LIBEX   void               cape_str_del           (CapeString*);                                 // release memory

__CAPE_LIBEX   CapeString         cape_str_sub           (const CapeString, number_t len);              // copy a part of the substring

__CAPE_LIBEX   CapeString         cape_str_uuid          (void);                                        // create an UUID and copy it into the string

__CAPE_LIBEX   CapeString         cape_str_fmt           (const CapeString format, ...);                // format to string

__CAPE_LIBEX   CapeString         cape_str_flp           (const CapeString format, va_list);            // format to string

__CAPE_LIBEX   number_t           cape_str_len           (const CapeString);                            // string length in characters

__CAPE_LIBEX   number_t           cape_str_size          (const CapeString);                            // string length in bytes

__CAPE_LIBEX   int                cape_str_empty         (const CapeString);                            // string length in bytes

__CAPE_LIBEX   int                cape_str_not_empty     (const CapeString);                            // string length in bytes

//-----------------------------------------------------------------------------

__CAPE_LIBEX   int                cape_str_equal         (const CapeString, const CapeString);          // case sensitive

__CAPE_LIBEX   int                cape_str_compare       (const CapeString, const CapeString);          // no case sensitive

__CAPE_LIBEX   int                cape_str_begins        (const CapeString, const CapeString);

//-----------------------------------------------------------------------------

__CAPE_LIBEX   CapeString         cape_str_catenate_c    (const CapeString, char c, const CapeString);

__CAPE_LIBEX   CapeString         cape_str_catenate_2    (const CapeString, const CapeString);

__CAPE_LIBEX   CapeString         cape_str_catenate_3    (const CapeString, const CapeString, const CapeString);

//-----------------------------------------------------------------------------

__CAPE_LIBEX   void               cape_str_replace_cp    (CapeString*, const CapeString source);      // replaces the object with a const string

__CAPE_LIBEX   void               cape_str_replace_mv    (CapeString*, CapeString*);                  // replaces the object with another object
  
//-----------------------------------------------------------------------------

__CAPE_LIBEX   void               cape_str_fill          (CapeString, number_t len, const CapeString source);       // will cut the content if not enough memory

__CAPE_LIBEX   void               cape_str_to_upper      (CapeString);

__CAPE_LIBEX   void               cape_str_to_lower      (CapeString);

//-----------------------------------------------------------------------------

#endif


