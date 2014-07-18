#ifndef SHM_RB_TREE_H
#define SHM_RB_TREE_H

/*
 * NOTE: this header is for internal implementation used and unitest used
 * so i expose all data structure here for convenience
 *
 * nil of rb_tree is very important, it might seen to be of nouse in normal 
 * binary-tree operation, but when it comes to red-black tree fixup operation,
 * nil helps a lot in boundary checking
 */

typedef int (*less_fn)(void* left, void* right);
typedef void (*release_fn)(void* data);

typedef enum {
  red,
  black,
} rb_color;

struct rb_node {
  struct rb_node* p;
  struct rb_node* l;
  struct rb_node* r;
  rb_color c;
  void* data;
};

struct rb_tree {
  struct rb_node* root;
  struct rb_node* nil;
  size_t          count;
  less_fn         less;
  release_fn      release;
};

/*
 * create a new tree with less and release callback provided
 */
struct rb_tree* rb_tree_new(less_fn less, release_fn release);

/*
 * free all resources, if release callback is provided, it will be call on each
 * element of the tree
 */
void rb_tree_free(struct rb_tree* t);

/*
 * add a new data
 */
int rb_tree_add(struct rb_tree* t, void* data);

/*
 * this api is not seen to be straght forward to be use:
 * 1. data is a pptr which will be use in less function for node searching,
 * point is: data need not to be complete as long as it has its 'key' element,
 * which decided the order-statistic of data.
 *
 * 2. when the funciton return, if specific data is found in rb_tree, the data
 * pptr will be override with the data ptr in the tree, now data pptr can be
 * used to do the update job
 *
 * note: don't change the 'key' part, or else the world is doom.
 */
int rb_tree_query(struct rb_tree* t, void** data);

#endif // SHM_RB_TREE_H

