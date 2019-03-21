#ifndef __CAPE_STC__STR__H
#define __CAPE_STC__STR__H 1

#include "sys/cape_export.h"
#include "sys/cape_types.h"

//=============================================================================

#define CapeString char*

__CAPE_LIBEX   CapeString         cape_str_cp            (const CapeString);                          // allocate memory and initialize the object

__CAPE_LIBEX   void               cape_str_del           (CapeString*);                               // release memory

__CAPE_LIBEX   CapeString         cape_str_sub           (const CapeString, number_t len);            // copy a part of the substring

__CAPE_LIBEX   CapeString         cape_str_uuid          (void);                                      // create an UUID and copy it into the string

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


