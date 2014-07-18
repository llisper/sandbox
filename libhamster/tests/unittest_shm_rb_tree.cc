#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#include "gtest/gtest.h"

extern "C" {
#include "shm_rb_tree.h"
#include "shm_error.h"
}

#define SEQ_NUM 10
#define nil(t) t->nil

struct p_info {
  p_info(int i = -1) : id(i) {}
  int id;
};

/*
 * data:
 *  1. random generated sequence
 *  2. worse-case sequence
 */
class shm_rb_tree_test : public ::testing::Test {
 protected:
  static void SetUpTestCase() {
    srand(time(NULL));
    rand_data_ = worse_case_data_ = NULL;
    t_ = NULL;
    generate(SEQ_NUM);
  }

  static void TearDownTestCase() {
    delete [] rand_data_;
    delete [] worse_case_data_;
    rb_tree_free(t_);
    rb_tree_free(wc_t_);
  }

  virtual void SetUp() {}
  virtual void TearDown() {}

  static void generate(size_t count) {
    int flags[100] = {0};
    int next;

    /*
    int input[] = { 95, 68, 63, 85, 38, 97, 47, 53, 10, 58, };
    for (; i < sizeof(input) / sizeof(input[0]); ++i)
      array[i].id = input[i];
    */

    printf("random generated: ");
    rand_data_ = new p_info[count];
    for (size_t i = 0; i < count; i++) {
      while (1) {
        if (flags[next = rand() % 100] == 0) {
          flags[next] = 1;
          break;
        }
      }
      rand_data_[i].id = next;
      printf("%d ", next);
    }
    printf("\n");

    printf("worse-case: ");
    worse_case_data_ = new p_info[count];
    for (size_t i = 0; i < count; i++) {
      worse_case_data_[i].id = i;
      printf("%d ", worse_case_data_[i].id);
    }
    printf("\n");
  }

  static p_info* rand_data_;
  static p_info* worse_case_data_;
  static rb_tree* t_;
  static rb_tree* wc_t_;
};

p_info* shm_rb_tree_test::rand_data_;
p_info* shm_rb_tree_test::worse_case_data_;
rb_tree* shm_rb_tree_test::t_;
rb_tree* shm_rb_tree_test::wc_t_;

typedef shm_rb_tree_test fixture;

static int less(void* left, void* right) {
  return ((p_info*)left)->id < ((p_info*)right)->id;
}

static void release(void* data) {
  delete (p_info*)data;
}

static p_info* data_ptr(rb_node* n) {
  return (p_info*)n->data;
}

static int data_id(rb_node* n) {
  return data_ptr(n)->id;
}

static void tree_guarantee(rb_tree* t, rb_node* n) {
  if (n == nil(t)) return;

  if (n->l != nil(t)) {
    ASSERT_GE(data_id(n), data_id(n->l));
  } else if (n->r != nil(t)) {
    ASSERT_LE(data_id(n), data_id(n->r));
  }

  tree_guarantee(t, n->l);
  tree_guarantee(t, n->r);
}

static void red_black_prop4(rb_tree* t, rb_node* n) {
  if (n == nil(t)) return;
  if (n->c == red) {
    ASSERT_EQ(n->l->c, black);
    ASSERT_EQ(n->r->c, black);
  }
  red_black_prop4(t, n->l);
  red_black_prop4(t, n->r);
}

static void red_black_prop5(rb_tree* t, rb_node* n, 
                            int& final_count, int black_count = 0) {
  if (n == nil(t)) {
    if (final_count == -1) {
      final_count = black_count;
    } else {
      ASSERT_EQ(final_count, black_count);
    }
    return;
  }

  if (n->c == black)
    black_count++;

  red_black_prop5(t, n->l, final_count, black_count);
  red_black_prop5(t, n->r, final_count, black_count);
}

static void red_black_prop_guarantee(rb_tree* t) {
  /* red_black tree key properties:
   *  1. every node is either red or black [ trivial to check ]
   *  2. the root is black
   *  3. every leaf(nil) is black [ trivial to check ]
   *  4. if a node is red, both of its children are black
   *  5. for each node, all paths from the node to decendent leaves contain the
   *  same number os black nodes
   */
  ASSERT_EQ(t->root->c, black);
  red_black_prop4(t, t->root);
  int final_count = -1;
  red_black_prop5(t, t->root, final_count);
}

TEST_F(shm_rb_tree_test, init) {
  fixture::t_ = rb_tree_new(less, release);
  ASSERT_NE(fixture::t_, (rb_tree*)NULL);
  fixture::wc_t_ = rb_tree_new(less, release);
  ASSERT_NE(fixture::wc_t_, (rb_tree*)NULL);
}

TEST_F(shm_rb_tree_test, rand_add) {
  for (int i = 0; i < SEQ_NUM; i++) {
    ASSERT_EQ(E_SHM_OK, 
              rb_tree_add(fixture::t_, new p_info(fixture::rand_data_[i])));
  }

  tree_guarantee(fixture::t_, fixture::t_->root);
  red_black_prop_guarantee(fixture::t_);
}

TEST_F(shm_rb_tree_test, worse_case_add) {
  for (int i = 0; i < SEQ_NUM; i++) {
    ASSERT_EQ(E_SHM_OK,
              rb_tree_add(fixture::wc_t_,
                          new p_info(fixture::worse_case_data_[i])));
  }

  tree_guarantee(fixture::wc_t_, fixture::wc_t_->root);
  red_black_prop_guarantee(fixture::wc_t_);
}

/*
TEST_F(shm_rb_tree_test, binary_tree_guarantee) {
  tree_guarantee(fixture::t_, fixture::t_->root);
  tree_guarantee(fixture::wc_t_, fixture::wc_t_->root);
}
*/

extern "C" {
void rb_tree_left_rotate(struct rb_tree* t, struct rb_node* n);
void rb_tree_right_rotate(struct rb_tree* t, struct rb_node* y);
}

static void fill_tree(rb_tree** t, int n, ...) {
  *t = rb_tree_new(less, release);
  ASSERT_NE(*t, (rb_tree*)NULL);

  va_list vl;
  va_start(vl, n);
  for (int i = 0; i < n; ++i) {
    ASSERT_EQ(E_SHM_OK, rb_tree_add(*t, new p_info(va_arg(vl, int))));
  }
  va_end(vl);
}

extern bool do_add_fixup;

TEST_F(shm_rb_tree_test, rotation_test) {
  do_add_fixup = false;
  rb_tree* t;
  fill_tree(&t, 5, 10, 5, 4, 7, 8);

  // 1. without node b
  rb_tree_left_rotate(t, t->root->l);
  ASSERT_EQ(data_id(t->root->l), 7);
  ASSERT_EQ(data_id(t->root->l->l), 5);
  ASSERT_EQ(t->root->l->l->r, nil(t));

  rb_tree_right_rotate(t, t->root->l);
  ASSERT_EQ(data_id(t->root->l), 5);
  ASSERT_EQ(data_id(t->root->l->r), 7);
  ASSERT_EQ(t->root->l->r->l, nil(t));

  ASSERT_EQ(E_SHM_OK, rb_tree_add(t, new p_info(6)));
  // 2. normal case
  rb_tree_left_rotate(t, t->root->l);
  ASSERT_EQ(data_id(t->root->l), 7);
  ASSERT_EQ(data_id(t->root->l->l), 5);
  ASSERT_EQ(data_id(t->root->l->l->r), 6);

  rb_tree_right_rotate(t, t->root->l);
  ASSERT_EQ(data_id(t->root->l), 5);
  ASSERT_EQ(data_id(t->root->l->r), 7);
  ASSERT_EQ(data_id(t->root->l->r->l), 6);

  // 3. root (no_parent)
  rb_tree_right_rotate(t, t->root);
  ASSERT_EQ(data_id(t->root), 5);
  ASSERT_EQ(data_id(t->root->r), 10);
  ASSERT_EQ(data_id(t->root->r->l), 7);

  rb_tree_left_rotate(t, t->root);
  ASSERT_EQ(data_id(t->root), 10);
  ASSERT_EQ(data_id(t->root->l), 5);
  ASSERT_EQ(data_id(t->root->l->r), 7);

  rb_tree_free(t);
  do_add_fixup = true;
}

#undef nil

