#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "shm_error.h"
#include "shm_config.h"
#include "shm_rb_tree.h"

#define nil(t) t->nil

#ifdef UNITTEST
shm_internal bool do_add_fixup = true;
#endif

shm_internal struct rb_node* node_new(struct rb_node* p, struct rb_node* nil, 
                                      rb_color c, void* data) {
  struct rb_node* n = (struct rb_node*)calloc(1, sizeof(struct rb_node));
  if (NULL != n) {
    n->p = p;
    n->l = n->r = nil;
    n->c = c;
    n->data = data;
  }
  return n;
}

shm_internal void rb_tree_query_internal(struct rb_tree* t, 
                                         void* data,
                                         struct rb_node** n,
                                         struct rb_node** p);

shm_internal void free_nodes(struct rb_tree* t,
                             struct rb_node* n,
                             release_fn release);

/** rotation **/
shm_internal void rb_tree_left_rotate(struct rb_tree* t, struct rb_node* n);
shm_internal void rb_tree_right_rotate(struct rb_tree* t, struct rb_node* y);

/** fixup **/
shm_internal void rb_tree_fixup(struct rb_tree* t, struct rb_node* new_node);

struct rb_tree* rb_tree_new(less_fn less, release_fn release) {
  struct rb_tree* t = NULL;

  if (NULL == less)
    return NULL;

  if (NULL != (t =  (struct rb_tree*)calloc(1, sizeof(struct rb_tree)))) {
    t->nil = node_new(NULL, NULL, black, NULL);
    t->less = less;
    t->release = release;
  }

  return t;
}

void rb_tree_free(struct rb_tree* t) {
  free_nodes(t, t->root, t->release);
  free(t->nil);
  free(t);
}

int rb_tree_add(struct rb_tree* t, void* data) {
  struct rb_node *found = NULL, *parent = NULL, *new_node = NULL;

  rb_tree_query_internal(t, data, &found, &parent);
  if (found != nil(t))
    return E_SHM_SAME_KEY_EXIST;

  new_node = node_new(parent, nil(t), red, data);
  if (NULL == new_node)
    return E_SHM_SYSTEM;

  if (parent == nil(t)) {
    t->root = new_node;
  } else {
    if (t->less(data, parent->data))
      parent->l = new_node;
    else
      parent->r = new_node;
  }

#ifdef UNITTEST
  if (do_add_fixup)
#endif
  rb_tree_fixup(t, new_node);
  ++(t->count);
  return 0;
}

int rb_tree_query(struct rb_tree* t, void** data) {
  struct rb_node *found = NULL, *parent = NULL;
  rb_tree_query_internal(t, *data, &found, &parent);
  if (found == nil(t))
    return E_SHM_KEY_NOT_FOUND;
  else
    *data = found->data;
  return E_SHM_OK;
}

shm_internal void rb_tree_query_internal(struct rb_tree* t, 
                                         void* data, 
                                         struct rb_node** n, 
                                         struct rb_node** p) {
  struct rb_node *cur = NULL, *parent = NULL;

  *n = *p = nil(t);
  if (NULL == t->root)
    return;

  parent = t->root->p;
  cur = t->root;

  while (cur != nil(t)) {
    if (t->less(data, cur->data)) {
      parent = cur;
      cur = cur->l;
    } else if (t->less(cur->data, data)) {
      parent = cur;
      cur = cur->r;
    } else {
      break;
    }
  }

  *n = cur;
  *p = parent;
}

shm_internal void free_nodes(struct rb_tree* t, 
                             struct rb_node* n,
                             release_fn release) {
  if (n == NULL || n == nil(t)) return;

  free_nodes(t, n->l, release);
  free_nodes(t, n->r, release);

  if (release != NULL)
    release(n->data);
  free(n);
}

/*
 *     n             y
 *    / \           / \
 *   a   y     =>  n   c
 *      / \       / \
 *     b   c     a   b
 */
shm_internal void rb_tree_left_rotate(struct rb_tree* t, struct rb_node* n) {
  struct rb_node* y = n->r;

  if (y == nil(t)) return;
  
  y->p = n->p;
  if (n->p != nil(t)) {
    if (n == n->p->l)
      n->p->l = y;
    else
      n->p->r = y;
  } else {
    t->root = y;
  }

  n->p = y;
  n->r = y->l;
  if (y->l != nil(t))
    y->l->p = n;
  y->l = n;
}

/*
 *     n             y
 *    / \           / \
 *   a   y     <=  n   c
 *      / \       / \
 *     b   c     a   b
 */
shm_internal void rb_tree_right_rotate(struct rb_tree* t, struct rb_node* y) {
  struct rb_node* n = y->l;

  if (n == nil(t)) return;

  n->p = y->p;
  if (y->p != nil(t)) {
    if (y == y->p->l)
      y->p->l = n;
    else
      y->p->r = n;
  } else {
    t->root = n;
  }

  y->p = n;
  y->l = n->r;
  if (n->r != nil(t))
    n->r->p = y;
  n->r = y;
}

shm_internal void rb_tree_fixup(struct rb_tree* t, struct rb_node* z)
{
  struct rb_node* y = NULL;

  while (z->p->c == red) {
    if (z->p == z->p->p->l) {
      y = z->p->p->r;
      if (y->c == red) {
        /** case 1 **/
        z->p->c = black;
        z->p->p->c = red;
        y->c = black;
        z = z->p->p;
      } else {
        if (z == z->p->r) {
          /** case 2 **/
          z = z->p;
          rb_tree_left_rotate(t, z);
        }
        /** case 3 **/
        z->p->c = black;
        z->p->p->c = red;
        rb_tree_right_rotate(t, z->p->p);
      }
    } else {
      y = z->p->p->l;
      if (y->c == red) {
        /** case 1 **/
        z->p->c = black;
        z->p->p->c = red;
        y->c = black;
        z = z->p->p;
      } else {
        if (z == z->p->l) {
          /** case 2 **/
          z = z->p;
          rb_tree_right_rotate(t, z);
        }
        /** case 3 **/
        z->p->c = black;
        z->p->p->c = red;
        rb_tree_left_rotate(t, z->p->p);
      }
    }
  }
  t->root->c = black;
}

#undef nil

