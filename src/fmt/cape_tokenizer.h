#ifndef __CAPE_FMT__TOKENIZER__H
#define __CAPE_FMT__TOKENIZER__H 1

#include "sys/cape_export.h"
#include "sys/cape_types.h"
#include "stc/cape_list.h"
#include "stc/cape_str.h"

//-----------------------------------------------------------------------------

                                  /* returns a list of CapeString's */
__CAPE_LIBEX   CapeList           cape_tokenizer_buf           (const char* buffer, number_t len, char token);

                                  /* returns TRUE / FALSE if successfull */
__CAPE_LIBEX   int                cape_tokenizer_split         (const CapeString source, char token, CapeString* p_left, CapeString* p_right);

//-----------------------------------------------------------------------------

__CAPE_LIBEX   CapeList           cape_tokenizer_str_all       (const CapeString haystack, const CapeString needle);

//-----------------------------------------------------------------------------

#endif


