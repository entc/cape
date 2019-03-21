#include "cape_udc.h"

// cape includes
#include "sys/cape_types.h"
#include "stc/cape_list.h"
#include "stc/cape_map.h"

//-----------------------------------------------------------------------------

struct CapeUdc_s
{
  u_t type;
  
  void* data;
  
  CapeString name;
};

//----------------------------------------------------------------------------------------

static void __STDCALL cape_udc_node_onDel (void* key, void* val)
{
  CapeUdc h = val; cape_udc_del (&h);
}

//----------------------------------------------------------------------------------------

static void __STDCALL cape_udc_list_onDel (void* ptr)
{
  CapeUdc h = ptr; cape_udc_del (&h);
}

//-----------------------------------------------------------------------------

CapeUdc cape_udc_new (u_t type, const CapeString name)
{
  CapeUdc self = CAPE_NEW(struct CapeUdc_s);
  
  self->type = type;
  self->data = NULL;
  
  self->name = cape_str_cp (name);
  
  switch (self->type)
  {
    case CAPE_UDC_NODE:
    {
      self->data = cape_map_new (NULL, cape_udc_node_onDel);
      break;
    }
    case CAPE_UDC_LIST:
    {
      self->data = cape_list_new (cape_udc_list_onDel);
      break;
    }
    case CAPE_UDC_STRING:
    {
      self->data = NULL;
      break;
    }
    case CAPE_UDC_FLOAT:
    {
      self->data = CAPE_NEW(double);
      break;
    }
  }
  
  return self;
}

//-----------------------------------------------------------------------------

void cape_udc_del (CapeUdc* p_self)
{
  CapeUdc self = *p_self;
  
  if (self == NULL)
  {
    return;
  }
  
  cape_str_del (&(self->name));
  
  switch (self->type)
  {
    case CAPE_UDC_NODE:
    {
      cape_map_del ((CapeMap*)&(self->data));
      break;
    }
    case CAPE_UDC_LIST:
    {
      cape_list_del ((CapeList*)&(self->data));
      break;
    }
    case CAPE_UDC_STRING:
    {
      cape_str_del ((CapeString*)&(self->data));
      break;
    }
    case CAPE_UDC_FLOAT:
    {
      CAPE_DEL(&(self->data), double);
      break;
    }
  }
  
  CAPE_DEL(p_self, struct CapeUdc_s);
}

//-----------------------------------------------------------------------------

CapeUdc cape_udc_cp (const CapeUdc self)
{
  
}

//-----------------------------------------------------------------------------

const CapeString  cape_udc_name (const CapeUdc self)
{
  return self->name;
}

//-----------------------------------------------------------------------------

u_t cape_udc_type (const CapeUdc self)
{
  return self->type;
}

//-----------------------------------------------------------------------------

void* cape_udc_data (const CapeUdc self)
{
  switch (self->type)
  {
    case CAPE_UDC_STRING:
    {
      return self->data;
    }
    case CAPE_UDC_FLOAT:
    {
      return self->data;
    }
    case CAPE_UDC_NUMBER:
    {
      return &(self->data);
    }
    case CAPE_UDC_BOOL:
    {
      return &(self->data);
    }
  }
  
  return NULL;
}

//-----------------------------------------------------------------------------

number_t cape_udc_size (const CapeUdc self)
{
  switch (self->type)
  {
    case CAPE_UDC_NODE:
    {
      return cape_map_size (self->data);
    }
    case CAPE_UDC_LIST:
    {
      return cape_list_size (self->data);
    }
    default:
    {
      return 0;
    }
  }
}

//-----------------------------------------------------------------------------

CapeUdc cape_udc_add (CapeUdc self, CapeUdc* p_item)
{
  switch (self->type)
  {
    case CAPE_UDC_NODE:
    {
      CapeUdc h = *p_item; 
      
      cape_map_insert (self->data, h->name, h);
      
      *p_item = NULL;
      
      return h;
    }
    case CAPE_UDC_LIST:
    {
      CapeUdc h = *p_item; 
      
      cape_list_push_back (self->data, h);
      
      *p_item = NULL;
      
      return h;
    }
    default:
    {
      // we can't add this item, but we can delete it
      cape_udc_del (p_item);
      
      return NULL;
    }
  }
}

//-----------------------------------------------------------------------------

void cape_udc_set_name (const CapeUdc self, const CapeString name)
{
  cape_str_replace_cp (&(self->name), name);  
}

//-----------------------------------------------------------------------------

CapeUdc cape_udc_add_name (CapeUdc self, CapeUdc* p_item, const CapeString name)
{
  cape_udc_set_name (*p_item, name);
  
  return cape_udc_add (self, p_item);
}

//-----------------------------------------------------------------------------

CapeUdc cape_udc_get (CapeUdc self, const CapeString name)
{
  switch (self->type)
  {
    case CAPE_UDC_NODE:
    {
      CapeMapNode n = cape_map_find (self->data, (void*)name);
      
      if (n)
      {
        return cape_map_node_value (n);
      }
      else
      {
        return NULL;
      }
    }
    default:
    {
      return NULL;
    }
  }
}

//-----------------------------------------------------------------------------

CapeUdc cape_udc_ext (CapeUdc self, const CapeString name)
{
  switch (self->type)
  {
    case CAPE_UDC_NODE:
    {
      CapeMapNode n = cape_map_find (self->data, (void*)name);
      
      if (n)
      {
        n = cape_map_extract (self->data, n);  
        
        CapeUdc h = cape_map_node_value (n);
        
        cape_map_node_del (&n);
        
        return h;
      }
      else
      {
        return NULL;
      }
    }
    default:
    {
      return NULL;
    }
  }  
}

//-----------------------------------------------------------------------------

void cape_udc_set_s_cp (CapeUdc self, const CapeString val)
{
  switch (self->type)
  {
    case CAPE_UDC_STRING:
    {
      cape_str_replace_cp ((CapeString*)&(self->data), val);
    }
  }   
}

//-----------------------------------------------------------------------------

void cape_udc_set_s_mv (CapeUdc self, CapeString* p_val)
{
  switch (self->type)
  {
    case CAPE_UDC_STRING:
    {
      cape_str_replace_mv ((CapeString*)&(self->data), p_val);
    }
  }   
}

//-----------------------------------------------------------------------------

void cape_udc_set_n (CapeUdc self, number_t val)
{
  switch (self->type)
  {
    case CAPE_UDC_NUMBER:
    {
      self->data = (void*)val;
    }
  }  
}

//-----------------------------------------------------------------------------

void cape_udc_set_f (CapeUdc self, double val)
{
  switch (self->type)
  {
    case CAPE_UDC_FLOAT:
    {
      double* h = self->data;
      
      *h = val;
    }
  }  
}

//-----------------------------------------------------------------------------

void cape_udc_set_b (CapeUdc self, int val)
{
  switch (self->type)
  {
    case CAPE_UDC_BOOL:
    {
      self->data = val ? (void*)1 : NULL;
    }
  }  
}

//-----------------------------------------------------------------------------

const CapeString cape_udc_s (CapeUdc self, const CapeString alt)
{
  switch (self->type)
  {
    case CAPE_UDC_STRING:
    {
      return self->data;
    }
    default:
    {
      return alt;
    }
  }    
}

//-----------------------------------------------------------------------------

CapeString cape_udc_s_mv (CapeUdc self, const CapeString alt)
{
  switch (self->type)
  {
    case CAPE_UDC_STRING:
    {
      CapeString h = self->data;
      
      self->data = NULL;
      
      return h;
    }
    default:
    {
      return cape_str_cp (alt);
    }
  }  
}

//-----------------------------------------------------------------------------

number_t cape_udc_n (CapeUdc self, number_t alt)
{
  switch (self->type)
  {
    case CAPE_UDC_NUMBER:
    {
      return (number_t)(self->data);
    }
    default:
    {
      return alt;
    }
  }  
}

//-----------------------------------------------------------------------------

double cape_udc_f (CapeUdc self, double alt)
{
  switch (self->type)
  {
    case CAPE_UDC_FLOAT:
    {
      double* h = self->data;
      
      return *h;
    }
    default:
    {
      return alt;
    }
  }  
}

//-----------------------------------------------------------------------------

int cape_udc_b (CapeUdc self, int alt)
{
  switch (self->type)
  {
    case CAPE_UDC_BOOL:
    {
      return self->data ? TRUE : FALSE;
    }
    default:
    {
      return alt;
    }
  }  
}

//-----------------------------------------------------------------------------

CapeUdc cape_udc_add_s_cp (CapeUdc self, const CapeString name, const CapeString val)
{
  CapeUdc h = cape_udc_new (CAPE_UDC_STRING, name);
  
  cape_udc_set_s_cp (h, val);
  
  return cape_udc_add (self, &h);
}

//-----------------------------------------------------------------------------

CapeUdc cape_udc_add_s_mv (CapeUdc self, const CapeString name, CapeString* p_val)
{
  CapeUdc h = cape_udc_new (CAPE_UDC_STRING, name);
  
  cape_udc_set_s_mv (h, p_val);
  
  return cape_udc_add (self, &h);
}

//-----------------------------------------------------------------------------

CapeUdc cape_udc_add_n (CapeUdc self, const CapeString name, number_t val)
{
  CapeUdc h = cape_udc_new (CAPE_UDC_NUMBER, name);
  
  cape_udc_set_n (h, val);
  
  return cape_udc_add (self, &h);
}

//-----------------------------------------------------------------------------

CapeUdc cape_udc_add_f (CapeUdc self, const CapeString name, double val)
{
  CapeUdc h = cape_udc_new (CAPE_UDC_FLOAT, name);
  
  cape_udc_set_f (h, val);
  
  return cape_udc_add (self, &h);
}

//-----------------------------------------------------------------------------

CapeUdc cape_udc_add_b (CapeUdc self, const CapeString name, int val)
{
  CapeUdc h = cape_udc_new (CAPE_UDC_BOOL, name);
  
  cape_udc_set_b (h, val);
  
  return cape_udc_add (self, &h);
}

//-----------------------------------------------------------------------------

CapeUdc cape_udc_add_node (CapeUdc self, const CapeString name)
{
  CapeUdc h = cape_udc_new (CAPE_UDC_NODE, name);
  
  return cape_udc_add (self, &h);
}

//-----------------------------------------------------------------------------

CapeUdc cape_udc_add_list (CapeUdc self, const CapeString name)
{
  CapeUdc h = cape_udc_new (CAPE_UDC_LIST, name);
  
  return cape_udc_add (self, &h);
}

//-----------------------------------------------------------------------------

const CapeString cape_udc_get_s (CapeUdc self, const CapeString name, const CapeString alt)
{
  CapeUdc h = cape_udc_get (self, name);
  
  if (h)
  {
    return cape_udc_s (h, alt);
  }
  
  return alt;
}

//-----------------------------------------------------------------------------

number_t cape_udc_get_n (CapeUdc self, const CapeString name, number_t alt)
{
  CapeUdc h = cape_udc_get (self, name);
  
  if (h)
  {
    return cape_udc_n (h, alt);
  }
  
  return alt;
}

//-----------------------------------------------------------------------------

double cape_udc_get_f (CapeUdc self, const CapeString name, double alt)
{
  CapeUdc h = cape_udc_get (self, name);
  
  if (h)
  {
    return cape_udc_f (h, alt);
  }
  
  return alt; 
}

//-----------------------------------------------------------------------------

int cape_udc_get_b (CapeUdc self, const CapeString name, int alt)
{
  CapeUdc h = cape_udc_get (self, name);
  
  if (h)
  {
    return cape_udc_b (h, alt);
  }
  
  return alt; 
}

//-----------------------------------------------------------------------------

CapeUdc cape_udc_get_node (CapeUdc self, const CapeString name)
{
  CapeUdc h = cape_udc_get (self, name);
  
  if (h)
  {
    if (h->type == CAPE_UDC_NODE)
    {
      return h;
    }
  }
  
  return NULL;  
}

//-----------------------------------------------------------------------------

CapeUdc cape_udc_get_list (CapeUdc self, const CapeString name)
{
  CapeUdc h = cape_udc_get (self, name);
  
  if (h)
  {
    if (h->type == CAPE_UDC_LIST)
    {
      return h;
    }
  }
  
  return NULL;   
}

//-----------------------------------------------------------------------------

CapeString cape_udc_ext_s (CapeUdc self, const CapeString name)
{
  switch (self->type)
  {
    case CAPE_UDC_NODE:
    {
      
    }
    default:
    {
      return NULL;
    }
  }  
}

//-----------------------------------------------------------------------------

CapeUdc cape_udc_ext_node (CapeUdc self, const CapeString name)
{
  switch (self->type)
  {
    case CAPE_UDC_NODE:
    {
      CapeUdc h = cape_udc_get (self, name);
      
      if (h)
      {
        if (h->type == CAPE_UDC_NODE)
        {
          return cape_udc_ext (self, name);
        }
      }
    }
    default:
    {
      return NULL;
    }
  }  
}

//-----------------------------------------------------------------------------

CapeUdc cape_udc_ext_list (CapeUdc self, const CapeString name)
{
  switch (self->type)
  {
    case CAPE_UDC_NODE:
    {
      CapeUdc h = cape_udc_get (self, name);
      
      if (h)
      {
        if (h->type == CAPE_UDC_LIST)
        {
          return cape_udc_ext (self, name);
        }
      }
    }
    default:
    {
      return NULL;
    }
  }  
}

//-----------------------------------------------------------------------------

CapeUdcCursor* cape_udc_cursor_new (CapeUdc self, int direction)
{
  CapeUdcCursor* cursor = CAPE_NEW(CapeUdcCursor);
  
  cursor->direction = direction;
  cursor->position = -1;
  cursor->item = NULL;
  
  switch (self->type)
  {
    case CAPE_UDC_NODE:
    {
      cursor->data = cape_map_cursor_create (self->data, direction);
      cursor->type = CAPE_UDC_NODE;
      
      break;
    }
    case CAPE_UDC_LIST:
    {
      cursor->data = cape_list_cursor_create (self->data, direction);
      cursor->type = CAPE_UDC_LIST;
      
      break;
    }
    default:
    {
      cursor->data = NULL;
      cursor->type = 0;
      
      break;
    }
  } 
  
  return cursor;
}

//-----------------------------------------------------------------------------

void cape_udc_cursor_del (CapeUdcCursor** p_cursor)
{
  CapeUdcCursor* cursor = *p_cursor;
  
  switch (cursor->type)
  {
    case CAPE_UDC_NODE:
    {
      cape_map_cursor_destroy ((CapeMapCursor**)&(cursor->data));
      break;
    }
    case CAPE_UDC_LIST:
    {
      cape_list_cursor_destroy ((CapeListCursor**)&(cursor->data));
      break;
    }
  } 
  
  CAPE_DEL(p_cursor, CapeUdcCursor);
}

//-----------------------------------------------------------------------------

int cape_udc_cursor_next (CapeUdcCursor* cursor)
{
  if (cursor->data)
  {
    switch (cursor->type)
    {
      case CAPE_UDC_NODE:
      {
        CapeMapCursor* c = cursor->data;
        
        int res = cape_map_cursor_next (c);
        
        if (res)
        {
          cursor->position++;
          cursor->item = cape_map_node_value (c->node);
        }
        
        return res;
      }
      case CAPE_UDC_LIST:
      {
        CapeListCursor* c = cursor->data;
        
        int res = cape_list_cursor_next (c);
        
        if (res)
        {
          cursor->position++;
          cursor->item = cape_list_node_data (c->node);
        }
        
        return res;
      }
    }
  }
  
  return FALSE;
}

//-----------------------------------------------------------------------------

int cape_udc_cursor_prev (CapeUdcCursor* cursor)
{
  if (cursor->data)
  {
    switch (cursor->type)
    {
      case CAPE_UDC_NODE:
      {
        CapeMapCursor* c = cursor->data;
        
        int res = cape_map_cursor_prev (c);
        
        if (res)
        {
          cursor->position--;
          cursor->item = cape_map_node_value (c->node);
        }
        
        return res;
      }
      case CAPE_UDC_LIST:
      {
        CapeListCursor* c = cursor->data;
        
        int res = cape_list_cursor_prev (c);
        
        if (res)
        {
          cursor->position--;
          cursor->item = cape_list_node_data (c->node);
        }
        
        return res;
      }
    }
  }
  
  return FALSE;  
}

//-----------------------------------------------------------------------------

void cape_udc_cursor_rm (CapeUdc self, CapeUdcCursor* cursor)
{
  
}

//-----------------------------------------------------------------------------

CapeUdc cape_udc_cursor_ext (CapeUdc self, CapeUdcCursor* cursor)
{
  switch (self->type)
  {
    case CAPE_UDC_NODE:
    {
      CapeMapNode n = cape_map_cursor_extract (self->data, cursor->data);
      
      CapeUdc h = cape_map_node_value (n);
      
      cape_map_node_del (&n);
      
      return h;
    }
    case CAPE_UDC_LIST:
    {
      return cape_list_cursor_extract (self->data, cursor->data);
      break;
    }
  }
  
  return NULL;
}

//-----------------------------------------------------------------------------

void cape_udc_print (const CapeUdc self) 
{
  switch (self->type)
  {
    case CAPE_UDC_NODE:
    {
      
      break;
    }
    case CAPE_UDC_LIST:
    {

      
      break;
    }
    case CAPE_UDC_STRING:
    {
      printf ("UDC [string] : %s\n", (char*)self->data);
      
      break;
    }
    case CAPE_UDC_NUMBER:
    {
      printf ("UDC [number] : %ld\n", (number_t)(self->data));
            
      break;
    }
  }
}

//-----------------------------------------------------------------------------
