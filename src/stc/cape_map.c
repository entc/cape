#include "cape_map.h"

// cape includes
#include "sys/cape_types.h"

//=============================================================================

#define CAPE_MAP_RED     1
#define CAPE_MAP_BLACK   0

#define CAPE_MAP_LEFT    0
#define CAPE_MAP_RIGHT   1

static const int CAPE_MAP_INV[2] = {CAPE_MAP_RIGHT, CAPE_MAP_LEFT};

//-----------------------------------------------------------------------------

struct CapeMapNode_s
{
  int color;
  
  CapeMapNode parent;
  CapeMapNode link[2];
  
  // content
  void* val;
  void* key;
};

//-----------------------------------------------------------------------------

CapeMapNode cape_map_node_new (CapeMapNode parent, void* key, void* val)
{
  CapeMapNode self = CAPE_NEW(struct CapeMapNode_s);
  
  self->color = CAPE_MAP_RED;
  
  self->parent = parent;
  self->link[CAPE_MAP_LEFT] = NULL;
  self->link[CAPE_MAP_RIGHT] = NULL;
  
  self->val = val;
  self->key = key;
  
  return self;
}

//-----------------------------------------------------------------------------

void cape_map_node_del (CapeMapNode* pself)
{
  CAPE_DEL(pself, struct CapeMapNode_s);
}

//-----------------------------------------------------------------------------

void* cape_map_node_value (CapeMapNode self)
{
  return self->val;
}

//-----------------------------------------------------------------------------

void* cape_map_node_key (CapeMapNode self)
{
  return self->key;
}

//-----------------------------------------------------------------------------

CapeMapNode cape_map_node_left (CapeMapNode n)
{
  return n == NULL ? NULL : n->link[CAPE_MAP_LEFT];
}

//-----------------------------------------------------------------------------

CapeMapNode cape_map_node_right (CapeMapNode n)
{
  return n == NULL ? NULL : n->link[CAPE_MAP_RIGHT];
}

//-----------------------------------------------------------------------------

CapeMapNode cape_map_node_toTheLast (CapeMapNode n, int dir)
{
  if (n)
  {
    while (n->link[dir])
    {
      n = n->link[dir];
    }
  }
  
  return n;
}

//-----------------------------------------------------------------------------

CapeMapNode cape_map_node_next (CapeMapNode n, int dir)
{
  CapeMapNode m = n->link[dir];
  CapeMapNode p = n->parent;
  
  if (m)
  {
    return cape_map_node_toTheLast (m, CAPE_MAP_INV[dir]);
  }
  
  while (p && p->link[dir] == n)
  {
    n = p;
    p = n->parent;
  }
  
  if (p)
  {
    return p;
  }
  
  return NULL;
}

//-----------------------------------------------------------------------------

CapeMapNode cape_map_siblingOf (CapeMapNode n)
{
  return (n == NULL || n->parent == NULL) ? NULL : (n == n->parent->link[CAPE_MAP_LEFT] ? n->parent->link[CAPE_MAP_RIGHT] : n->parent->link[CAPE_MAP_LEFT]);
}

//-----------------------------------------------------------------------------

CapeMapNode cape_map_grandparentOf (CapeMapNode n)
{
  return (n == NULL || n->parent == NULL) ? NULL : n->parent->parent;
}

//-----------------------------------------------------------------------------

int cape_map_isRed (CapeMapNode n)
{
  return n && n->color == CAPE_MAP_RED;
}

//-----------------------------------------------------------------------------

int cape_map_isBlack (CapeMapNode n)
{
  return n && n->color == CAPE_MAP_BLACK;
}

//-----------------------------------------------------------------------------

void cape_map_setColor (CapeMapNode n, int color)
{
  if (n)
  {
    n->color = color;
  }
}

//=============================================================================

struct CapeMap_s
{
  CapeMapNode root;
  
  fct_cape_map_cmp onCompare;
  
  fct_cape_map_destroy onDestroy;
  
  size_t size;
};

//-----------------------------------------------------------------------------

static int __STDCALL cape_map_node_cmp (const void* a, const void* b)
{
  const char* s1 = a;
  const char* s2 = b;
  
  return strcmp(s1, s2);
}

//-----------------------------------------------------------------------------

void cape_map_rotate (CapeMap self, CapeMapNode x, int dir)
{
  CapeMapNode y = x->link[CAPE_MAP_INV[dir]];
  if (y)
  {
    CapeMapNode h = y->link[dir];
    CapeMapNode p = x->parent;
    
    // turn y's left subtree into x's right subtree
    x->link[CAPE_MAP_INV[dir]] = h;
    y->link[dir] = x;
    
    if (h)
    {
      h->parent = x;
    }
    
    // link parent
    y->parent = x->parent;
    x->parent = y;
    
    // link parent's link
    if (p)
    {
      // set parents link
      if (p->link[CAPE_MAP_LEFT] == x)
      {
        p->link[CAPE_MAP_LEFT] = y;
      }
      else
      {
        p->link[CAPE_MAP_RIGHT] = y;
      }
    }
    else  // x was root node
    {
      self->root = y;
    }
  }
}

//-----------------------------------------------------------------------------

CapeMap cape_map_new (fct_cape_map_cmp onCompare, fct_cape_map_destroy onDestroy)
{
  CapeMap self = CAPE_NEW(struct CapeMap_s);
  
  self->root = NULL;
  self->size = 0;
  
  self->onCompare = onCompare ? onCompare : cape_map_node_cmp;
  self->onDestroy = onDestroy;
  
  return self;
}

//-----------------------------------------------------------------------------

void cape_map_del_node (CapeMap self, CapeMapNode* pself)
{
  CapeMapNode n = *pself;
  
  if (self->onDestroy)
  {
    self->onDestroy (n->key, n->val);
  }
  
  cape_map_node_del (pself);
}

//-----------------------------------------------------------------------------

void cape_map_clr (CapeMap self)
{
  CapeMapNode n = self->root;
  
  while (n)
  {
    CapeMapNode p;
    
    if (n->link[CAPE_MAP_LEFT])
    {
      p = n->link[CAPE_MAP_LEFT];
      
      // mark that we already went this way
      n->link[CAPE_MAP_LEFT] = NULL;
      
      n = p;
      
      continue;
    }
    
    if (n->link[CAPE_MAP_RIGHT])
    {
      p = n->link[CAPE_MAP_RIGHT];
      
      // mark that we already went this way
      n->link[CAPE_MAP_RIGHT] = NULL;
      
      n = p;
      
      continue;
    }
    
    // no links available -> delete
    p = n->parent;
    
    cape_map_del_node (self, &n);
    
    n = p;
  }
  
  self->root = NULL;
  self->size = 0;
}

//-----------------------------------------------------------------------------

void cape_map_del (CapeMap* pself)
{
  CapeMap self =  *pself;
  
  cape_map_clr (self);
  
  CAPE_DEL(pself, struct CapeMap_s);
}

//-----------------------------------------------------------------------------

CapeMapNode cape_map_find (CapeMap self, void* key)
{
  CapeMapNode n = self->root;
  
  while (n)
  {
    int res = self->onCompare (n->key, key);
    
    if (res < 0)
    {
      n = n->link[CAPE_MAP_RIGHT];
    }
    else if (res > 0)
    {
      n = n->link[CAPE_MAP_LEFT];
    }
    else
    {
      return n;
    }
  }
  
  return NULL;
}

//-----------------------------------------------------------------------------

void cape_map_node_swap (CapeMapNode n1, CapeMapNode n2)
{
  void* key = n1->key;
  void* val = n1->val;
  
  n1->key = n2->key;
  n1->val = n2->val;
  
  n2->key = key;
  n2->val = val;
}

//-----------------------------------------------------------------------------

CapeMapNode stdbst_insert (CapeMap self, CapeMapNode z)
{
  CapeMapNode y = NULL;
  CapeMapNode x = self->root;
  
  while (x)
  {
    int res;
    
    y = x;
    
    res = self->onCompare (z->key, x->key);
    if (res < 0)
    {
      x = x->link[CAPE_MAP_LEFT];
    }
    else if (res > 0)
    {
      x = x->link[CAPE_MAP_RIGHT];
    }
    else
    {
      cape_map_node_swap (x, z);
      return x;
    }
  }
  
  z->parent = y;
  
  if (y)
  {
    int res = self->onCompare (z->key, y->key);
    if (res < 0)
    {
      y->link[CAPE_MAP_LEFT] = z;
    }
    else if (res > 0)
    {
      y->link[CAPE_MAP_RIGHT] = z;
    }
    else
    {
      cape_map_node_swap (y, z);
      return y;
    }
  }
  else
  {
    self->root = z;
  }
  
  self->size++;
  
  return z;
}

//-----------------------------------------------------------------------------

void cape_map_insert_balance (CapeMap self, CapeMapNode x)
{
  CapeMapNode n = x;
  n->color = CAPE_MAP_RED;
  
  while (n && cape_map_isRed (n->parent))
  {
    CapeMapNode p = n->parent;   // p is always valid
    CapeMapNode s = cape_map_siblingOf (p);
    
    if (cape_map_isRed (s))
    {
      p->color = CAPE_MAP_BLACK;
      s->color = CAPE_MAP_BLACK;
      
      s->parent->color = CAPE_MAP_RED;
      n = s->parent;
    }
    else
    {
      CapeMapNode g = p->parent;
      if (g)
      {
        if (p == g->link[CAPE_MAP_LEFT])
        {
          if (n == p->link[CAPE_MAP_RIGHT])
          {
            n = p;
            cape_map_rotate (self, n, CAPE_MAP_LEFT);
          }
          
          // reset
          p = n->parent;   // p is always valid
          g = p->parent;   // might be NULL
          
          p->color = CAPE_MAP_BLACK;
          
          if (g)
          {
            g->color = CAPE_MAP_RED;
            cape_map_rotate (self, g, CAPE_MAP_RIGHT);
          }
        }
        else
        {
          if (n == p->link[CAPE_MAP_LEFT])
          {
            n = p;
            cape_map_rotate (self, n, CAPE_MAP_RIGHT);
          }
          
          // reset
          p = n->parent;   // p is always valid
          g = p->parent;   // might be NULL
          
          p->color = CAPE_MAP_BLACK;
          
          if (g)
          {
            g->color = CAPE_MAP_RED;
            cape_map_rotate (self, g, CAPE_MAP_LEFT);
          }
        }
      }
      else
      {
        p->color = CAPE_MAP_BLACK;
      }
    }
  }
  
  self->root->color = CAPE_MAP_BLACK;
}

//-----------------------------------------------------------------------------

CapeMapNode cape_map_insert (CapeMap self, void* key, void* val)
{
  CapeMapNode n = cape_map_node_new (NULL, key, val);
  
  if (stdbst_insert (self, n) == n)
  {
    // re-balance the tree
    cape_map_insert_balance (self, n);
  }
  else
  {
    cape_map_del_node (self, &n);
  }
  
  return n;
}

//-----------------------------------------------------------------------------

int cape_map_maxdepth (CapeMapNode n)
{
  int depth = 0;
  
  if (n)
  {
    int dp = 1;
    int dl = 0;
    int dr = 0;
    
    if (n->link[CAPE_MAP_LEFT])
    {
      dl = cape_map_maxdepth (n->link[CAPE_MAP_LEFT]);
    }
    
    if (n->link[CAPE_MAP_RIGHT])
    {
      dr = cape_map_maxdepth (n->link[CAPE_MAP_LEFT]);
    }
    
    if (dr > dl)
    {
      return dp + dr;
    }
    else
    {
      return dp + dl;
    }
  }
  
  return depth;
}

//-----------------------------------------------------------------------------

int cape_map_maxtreedepth (CapeMap self)
{
  return cape_map_maxdepth (self->root);
}

//-----------------------------------------------------------------------------

void cape_map_tree_remove_balance (CapeMap self, CapeMapNode x)
{
  CapeMapNode p = x->parent;
  CapeMapNode s;
  int d;
  
  if (p == NULL)
  {
    // Case 1 - node is the new root.
    
    return;
  }
  
  // get sibling
  if (p->link[CAPE_MAP_LEFT] == x)
  {
    d = CAPE_MAP_RIGHT;
  }
  else
  {
    d = CAPE_MAP_LEFT;
  }
  
  s = p->link[d];
  
  if (s)
  {
    if (s->color == CAPE_MAP_RED)
    {
      // Case 2 - sibling is red.
      
      p->color = CAPE_MAP_RED;
      s->color = CAPE_MAP_BLACK;
      
      cape_map_rotate (self, p, CAPE_MAP_INV[d]);
      
      // Rotation, need to update parent/sibling
      //cape_map_rotate (self, p, d);
      
      p = x->parent;
      
      if (p)
      {
        // get sibling
        if (p->link[CAPE_MAP_LEFT] == x)
        {
          d = CAPE_MAP_RIGHT;
        }
        else
        {
          d = CAPE_MAP_LEFT;
        }
        
        s = p->link[d];
      }
      else
      {
        s = NULL;
      }
    }
    
    if (s && (s->color == CAPE_MAP_BLACK))
    {
      if (cape_map_isBlack (s->link[CAPE_MAP_RIGHT]) && cape_map_isBlack (s->link[CAPE_MAP_LEFT]))
      {
        if (p->color == CAPE_MAP_BLACK)
        {
          // Case 3 - parent, sibling, and sibling's children are black.
          s->color = CAPE_MAP_RED;
          
          cape_map_tree_remove_balance (self, p);
          
          return;
        }
        else
        {
          // Case 4 - sibling and sibling's children are black, but parent is red.
          s->color = CAPE_MAP_RED;
          p->color = CAPE_MAP_BLACK;
          
          return;
        }
      }
      else
      {
        // Case 5 - sibling is black, sibling's left child is red,
        // sibling's right child is black, and node is the left child of
        // its parent.
        if ((x == p->link[CAPE_MAP_LEFT]) && cape_map_isRed (s->link[CAPE_MAP_LEFT]) && cape_map_isBlack (s->link[CAPE_MAP_RIGHT]))
        {
          s->color = CAPE_MAP_RED;
          
          cape_map_rotate (self, p, CAPE_MAP_RIGHT);
          
          p = x->parent;
          
          if (p)
          {
            // get sibling
            if (p->link[CAPE_MAP_LEFT] == x)
            {
              d = CAPE_MAP_RIGHT;
            }
            else
            {
              d = CAPE_MAP_LEFT;
            }
            
            s = p->link[d];
          }
          else
          {
            s = NULL;
          }
        }
        else if ((x == p->link[CAPE_MAP_RIGHT]) && cape_map_isBlack (s->link[CAPE_MAP_LEFT]) && cape_map_isRed (s->link[CAPE_MAP_RIGHT]))
        {
          s->color = CAPE_MAP_RED;
          
          cape_map_rotate (self, p, CAPE_MAP_LEFT);
          
          p = x->parent;
          
          if (p)
          {
            // get sibling
            if (p->link[CAPE_MAP_LEFT] == x)
            {
              d = CAPE_MAP_RIGHT;
            }
            else
            {
              d = CAPE_MAP_LEFT;
            }
            
            s = p->link[d];
          }
          else
          {
            s = NULL;
          }
        }
      }
    }
    
    if (s)
    {
      // Case 6 - sibling is black, sibling's right child is red, and node
      // is the left child of its parent.
      s->color = p->color;
      p->color = CAPE_MAP_BLACK;
      
      if (x == p->link[CAPE_MAP_LEFT])
      {
        cape_map_setColor (s->link[CAPE_MAP_RIGHT], CAPE_MAP_BLACK);
        
        cape_map_rotate (self, p, CAPE_MAP_LEFT);
      }
      else
      {
        cape_map_setColor (s->link[CAPE_MAP_LEFT], CAPE_MAP_BLACK);
        
        cape_map_rotate (self, p, CAPE_MAP_RIGHT);
      }
    }
    
  }
  
  /*
   * 
   * 
   *   while (p && x->color == CAPE_MAP_BLACK)
   *   {
   *      int d;
   *      CapeMapNode s;
   * 
   *      if (p->link[CAPE_MAP_LEFT] == x)
   *      {
   *         d = CAPE_MAP_LEFT;
}
else
{
d = CAPE_MAP_RIGHT;
}

s = p->link[CAPE_MAP_INV[d]];

if (s)
{
if (s->color == CAPE_MAP_RED)
{
s->color = CAPE_MAP_BLACK;
p->color = CAPE_MAP_RED;

cape_map_rotate (self, p, d);

p = x->parent;
s = p->link[CAPE_MAP_INV[d]];
}

if ((s->link[CAPE_MAP_LEFT]->color == CAPE_MAP_BLACK) && (s->link[CAPE_MAP_RIGHT]->color == CAPE_MAP_BLACK))
{
s->color = CAPE_MAP_RED;
x = x->parent;
}
else
{
if (s->link[CAPE_MAP_INV[d]]->color == CAPE_MAP_BLACK)
{
s->link[CAPE_MAP_LEFT]->color = CAPE_MAP_BLACK;
s->color = CAPE_MAP_RED;

cape_map_rotate (self, s, CAPE_MAP_INV[d]);

s = x->parent->link[CAPE_MAP_INV[d]];
}

s->color = x->parent->color;
x->parent->color = CAPE_MAP_BLACK;
s->link[CAPE_MAP_INV[d]]->color = CAPE_MAP_BLACK;

cape_map_rotate (self, x->parent, d);
x = self->root;
}
}
else
{
break;
}
}
*/
}

//-----------------------------------------------------------------------------

void stdbst_replace (CapeMapNode a, CapeMapNode b)
{
  if (a != b)
  {
    int color;
    
    // swap content of nodes
    cape_map_node_swap (a, b);
    
    // swap the color
    color = a->color;
    a->color = b->color;
    b->color = color;
  }
}

//-----------------------------------------------------------------------------

CapeMapNode stdbst_delete (CapeMap self, CapeMapNode x)
{
  CapeMapNode n = x;
  CapeMapNode p;
  CapeMapNode c = NULL;
  CapeMapNode l = n->link[CAPE_MAP_LEFT];
  CapeMapNode r = n->link[CAPE_MAP_RIGHT];
  
  while (l && r)
  {
    if (r)
    {
      n = cape_map_node_next (n, CAPE_MAP_RIGHT);  // successor of n
    }
    else
    {
      n = cape_map_node_next (n, CAPE_MAP_LEFT);  // predecessor of n
    }
    
    l = n->link[CAPE_MAP_LEFT];
    r = n->link[CAPE_MAP_RIGHT];
  }
  
  stdbst_replace (n, x);
  
  if (l)
  {
    c = l;
    
  }
  else if (r)
  {
    c = r;
  }
  
  if (c)
  {
    stdbst_replace (n, c);
    
    n->link[CAPE_MAP_LEFT] = c->link[CAPE_MAP_LEFT];
    
    if (n->link[CAPE_MAP_LEFT])
    {
      n->link[CAPE_MAP_LEFT]->parent = n;
    }
    
    n->link[CAPE_MAP_RIGHT] = c->link[CAPE_MAP_RIGHT];
    
    if (n->link[CAPE_MAP_RIGHT])
    {
      n->link[CAPE_MAP_RIGHT]->parent = n;
    }
    
    if (n->color == CAPE_MAP_BLACK)
    {
      if (c->color == CAPE_MAP_RED)
      {
        c->color = CAPE_MAP_BLACK;
      }
      else
      {
        
      }
    }
    
    n = c;
  }
  else
  {
    p = n->parent;
    
    if (p)
    {
      // find on which link the node is connected
      if (p->link[CAPE_MAP_LEFT] == n)
      {
        p->link[CAPE_MAP_LEFT] = NULL;
      }
      else
      {
        p->link[CAPE_MAP_RIGHT] = NULL;
      }
    }
    else
    {
      self->root = NULL;  // last node
    }
  }
  
  self->size--;
  
  return n;
}

//-----------------------------------------------------------------------------

CapeMapNode cape_map_extract (CapeMap self, CapeMapNode node)
{
  if (node)
  {
    CapeMapNode x = stdbst_delete (self, node);
    
    /*
     *      if (node->color == CAPE_MAP_BLACK)
     *      {
     *         // re-balance the tree
     *         cape_map_tree_remove_balance (self, x);
  }
  */
    
    return x;
  }
  
  return node;
}

//-----------------------------------------------------------------------------

void cape_map_erase (CapeMap self, CapeMapNode node)
{
  if (node)
  {
    CapeMapNode node2 = cape_map_extract (self, node);
    
    cape_map_del_node (self, &node2);
  }
}

//-----------------------------------------------------------------------------

unsigned long cape_map_size (CapeMap self)
{
  return self->size;
}

//-----------------------------------------------------------------------------

int cape_map_validate_parent (CapeMapNode n, CapeMapNode parent)
{
  if (n == NULL)
  {
    return 1;
  }
  
  if (n->parent != parent)
  {
    return 0;
  }
  
  if (cape_map_validate_parent (n->link[CAPE_MAP_LEFT], n) == 0)
  {
    return 0;
  }
  
  if (cape_map_validate_parent (n->link[CAPE_MAP_RIGHT], n) == 0)
  {
    return 0;
  }
  
  return 1;
}

//-----------------------------------------------------------------------------

int cape_map_validate (CapeMap self)
{
  return cape_map_validate_parent (self->root, NULL);
}

//=============================================================================

void cape_map_cursor_init (CapeMap self, CapeMapCursor* cursor, int direction)
{
  if (direction == CAPE_DIRECTION_FORW)
  {
    cursor->node = cape_map_node_toTheLast (self->root, CAPE_MAP_LEFT);
  }
  else
  {
    cursor->node = cape_map_node_toTheLast (self->root, CAPE_MAP_RIGHT);
  }
  
  cursor->position = -1;
  cursor->direction = direction;
}

//-----------------------------------------------------------------------------

CapeMapCursor* cape_map_cursor_create (CapeMap self, int direction)
{
  CapeMapCursor* cursor = (CapeMapCursor*)malloc (sizeof(CapeMapCursor));
  
  cape_map_cursor_init (self, cursor, direction);
  
  return cursor;
}

//-----------------------------------------------------------------------------

void cape_map_cursor_destroy (CapeMapCursor** pcursor)
{
  CapeMapCursor* cursor = *pcursor;
  
  free (cursor);
  *pcursor = NULL;
}

//-----------------------------------------------------------------------------

int cape_map_cursor_nextnode (CapeMapCursor* cursor, int dir)
{
  if (cursor->position < 0)
  {
    if (dir == cursor->direction)
    {
      if (cursor->node)
      {
        cursor->position = 0;
        return 1;
      }
    }
  }
  else
  {
    CapeMapNode n = cursor->node;
    
    if (n)
    {
      cursor->node = cape_map_node_next (n, dir);
      
      if (cursor->node)
      {
        cursor->position++;
        return 1;
      }
    }
  }
  
  return 0;
}

//-----------------------------------------------------------------------------

int cape_map_cursor_next (CapeMapCursor* cursor)
{
  return cape_map_cursor_nextnode (cursor, CAPE_MAP_RIGHT);
}

//-----------------------------------------------------------------------------

int cape_map_cursor_prev (CapeMapCursor* cursor)
{
  return cape_map_cursor_nextnode (cursor, CAPE_MAP_LEFT);
}

//-----------------------------------------------------------------------------

CapeMapNode cape_map_cursor_extract (CapeMap self, CapeMapCursor* cursor)
{
  CapeMapNode x = cursor->node;
  
  if (x)
  {
    if (cape_map_cursor_prev (cursor))
    {
      cape_map_extract (self, x);
    }
    else
    {
      x = cape_map_extract (self, x);
      
      if (cursor->direction == CAPE_DIRECTION_FORW)
      {
        cursor->node = cape_map_node_toTheLast (self->root, CAPE_MAP_LEFT);
      }
      else
      {
        cursor->node = cape_map_node_toTheLast (self->root, CAPE_MAP_RIGHT);
      }
      
      cursor->position = -1;
    }
    
    return x;
  }
  else
  {
    return NULL;
  }
}

//-----------------------------------------------------------------------------

void cape_map_cursor_erase (CapeMap self, CapeMapCursor* cursor)
{
  CapeMapNode node2 = cape_map_cursor_extract (self, cursor);
  
  cape_map_del_node (self, &node2);
}

//-----------------------------------------------------------------------------


