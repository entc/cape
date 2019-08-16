#ifndef __CAPE_STC__HPP__H
#define __CAPE_STC__HPP__H 1

#include <stc/cape_stream.h>
#include <stc/cape_list.h>
#include <stc/cape_udc.h>

// STL includes
#include <limits>

namespace cape
{
  
  //-----------------------------------------------------------------------------------------------------
  
  struct ListHolder
  {
    ListHolder (CapeList obj) : m_obj (obj) {}
    
    ~ListHolder () { cape_list_del (&m_obj); }
    
    CapeList m_obj;
  };

  //-----------------------------------------------------------------------------------------------------

  // a traits prototype for UDC types
  template <typename T> struct UdcTransType 
  {
    static const int type = CAPE_UDC_UNDEFINED;     
    static void add (CapeUdc, const CapeString, const T&) {  }
  };
  
  template <> struct UdcTransType<int>
  {
    static const int type = CAPE_UDC_NUMBER;
    static void add (CapeUdc obj, const CapeString name, const int& value) { cape_udc_add_n (obj, name, value); }
  };
  
  template <> struct UdcTransType<long>
  {
    static const int type = CAPE_UDC_NUMBER;
    static void add (CapeUdc obj, const CapeString name, const long& value) { cape_udc_add_n (obj, name, value); }
  };

  template <> struct UdcTransType<double>
  {
    static const int type = CAPE_UDC_FLOAT;
    static void add (CapeUdc obj, const CapeString name, const double& value) { cape_udc_add_f (obj, name, value); }
  };

  template <> struct UdcTransType<bool>
  {
    static const int type = CAPE_UDC_BOOL;
    static void add (CapeUdc obj, const CapeString name, const bool& value) { cape_udc_add_b (obj, name, value ? TRUE : FALSE); }
  };

  template <> struct UdcTransType<char*>
  {
    static const int type = CAPE_UDC_STRING;
    static void add (CapeUdc obj, const CapeString name, const char*& value) { cape_udc_add_s_cp (obj, name, value); }
  };

  //-----------------------------------------------------------------------------------------------------

  struct Udc
  {
    Udc (CapeUdc obj) : m_obj (obj) {}
    
    // copy constructor
    Udc (const Udc& e) noexcept : m_obj (cape_udc_cp (e.m_obj)) {}
    
    // move constructor
    Udc (Udc&& e) noexcept : m_obj (cape_udc_mv (&(e.m_obj))) {}
    
    // destructor
    ~Udc () { cape_udc_del (&m_obj); }
    
    template <typename T> void add (const CapeString name, const T& val)
    {
      // use the traits for specialization
      UdcTransType<T>::add (m_obj, name, val);
    }

    // the cape object
    CapeUdc m_obj;
  };
  
  //-----------------------------------------------------------------------------------------------------

  struct UdcCursorHolder
  {
    UdcCursorHolder (CapeUdcCursor* obj) : m_obj (obj) {}

    ~UdcCursorHolder () { cape_udc_cursor_del (&m_obj); }

    bool next () { return cape_udc_cursor_next (m_obj); }
    
    CapeUdc item () { return m_obj->item; }
    
    CapeUdcCursor* m_obj;
  };
  
  //-----------------------------------------------------------------------------------------------------

  struct StringHolder
  {
    StringHolder (CapeString obj) : m_obj (obj) {}
    
    ~StringHolder () { cape_str_del (&m_obj); }
    
    CapeString m_obj;
  };

  //-----------------------------------------------------------------------------------------------------
  
  struct StreamHolder
  {
    StreamHolder ()  noexcept : obj (cape_stream_new ())
    {
    }
    
    // copy constructor
    StreamHolder (const StreamHolder& e) noexcept : obj (const_cast<StreamHolder&>(e).release())
    {
    }
    
    // move constructor
    StreamHolder (StreamHolder&& e) noexcept : obj (e.release())
    {
    }
    
    StreamHolder& operator=(const StreamHolder& e) = default;
    
    ~StreamHolder ()
    {
      cape_stream_del (&obj);
    }
    
    CapeStream release ()
    {
      CapeStream ret = obj;
      
      obj = NULL;
      
      return ret;
    }
    
    const char* data ()
    {
      return cape_stream_data (obj);
    }
    
    number_t size ()
    {
      return cape_stream_size (obj);
    }
    
    CapeStream obj;
    
  };
  
  //-----------------------------------------------------------------------------------------------------  
}

#endif
