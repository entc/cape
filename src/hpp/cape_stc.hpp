#ifndef __CAPE_STC__HPP__H
#define __CAPE_STC__HPP__H 1

#include <stc/cape_stream.h>
#include <stc/cape_list.h>

namespace cape
{
  
  struct ListHolder
  {
    ListHolder (CapeList obj) : m_obj (obj) {}
    
    ~ListHolder () { cape_list_del (&m_obj); }
    
    CapeList m_obj;
  };

  struct UdcHolder
  {
    UdcHolder (CapeUdc obj) : m_obj (obj) {}
    
    ~UdcHolder () { cape_udc_del (&m_obj); }
    
    CapeUdc m_obj;
  };
  
  struct StringHolder
  {
    StringHolder (CapeString obj) : m_obj (obj) {}
    
    ~StringHolder () { cape_str_del (&m_obj); }
    
    CapeString m_obj;
  };

  //======================================================================
  
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
  
  
}

#endif
