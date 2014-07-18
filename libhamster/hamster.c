#include <stdint.h>
#include <string.h>

#include "hamster.h"
#include "shm_crc32.h"
#include "shm_error.h"
#include "shm_config.h"
#include "shm_rb_tree.h"
#include "shm_segments.h"

struct h_value_t {
  /* pointer to value */
  void* ptr;
  /* size of this value */
  uint32_t size;
  /* max size of this value */
  uint32_t max_size; 
};

/*
 * layout
 * +-----------+-------------------------------------------+
 * |   header  |                                           |
 * +-----------+--+                                        |
 * |    key    |  |                                        |
 * +-----------+--|----------------+-------------------+   |
 * |           |  |-> data_size    |                   |   |-> total_size
 * |   value   |  |                |-> h_value_t.size  |   |
 * |           |  |                |                   |-> h_value_t.max_size
 * +-----------+--+----------------+                   |   |
 * |   unused  |                                       |   |
 * +-----------+---------------------------------------+---+
 */

struct shm_data_header {
  int checksum;
  uint32_t total_size;
  uint32_t data_size;
  struct shmseg_ptr_base next;
};

#define hdr_size sizeof(struct shm_data_header)

shm_internal uint32_t hdr_key_size(struct shm_data_header* hdr);
shm_internal const char* hdr_key(struct shm_data_header* hdr);
shm_internal uint32_t hdr_value_size(struct shm_data_header* hdr);
shm_internal uint32_t hdr_value_maxsize(struct shm_data_header* hdr);
shm_internal void* hdr_value(struct shm_data_header* hdr);

struct data_t {
  struct shmseg_ptr base_sptr;
  const char* key;
  struct h_value_t value;
};

shm_internal bool g_init;
shm_internal struct data_t* g_data_tail;
shm_internal struct rb_tree* g_data_tree;

shm_internal int  data_load(struct data_t** d, struct shmseg_ptr* base_sptr);
shm_internal int  data_add(struct data_t* data_ptr);
shm_internal int  data_less(void* left, void* right);
shm_internal void data_release(void* data) { free(data); }
shm_internal struct shm_data_header* data_hdr(struct data_t* d);
shm_internal int  data_checksum(struct shm_data_header* hdr);
shm_internal int  data_update(struct data_t* data_ptr, struct h_value_t* val);
shm_internal int  data_new(const char* key, struct h_value_t* val);
shm_internal void data_set_next(struct data_t* data_ptr, struct shmseg_ptr_base* base_sptr);

int hamster_init() {
  int ec = E_SHM_OK;
  struct shmseg_ptr sptr = { { -1, 0 }, NULL };
  struct data_t* data_ptr = NULL;
  struct shmseg_ptr_base end = { -1, 0 };

  if (g_init)
    return E_SHM_INIT_ONLY_ONCE;

  if (E_SHM_OK != (ec = shmseg_init(SHM_KEY)))
    return ec;

  if (NULL == (g_data_tree = rb_tree_new(data_less, data_release)))
    return E_SHM_TREE_NEW_FAILED;

  shmseg_ptr_reset(&sptr);
  if (E_SHM_OK != (ec = shmseg_first_ptr(&sptr))) {
    if (ec == E_SHM_EMPTY)
      ec = E_SHM_OK;
  } else {
    do {
      if (E_SHM_OK != (ec = data_load(&data_ptr, &sptr)) || 
          E_SHM_OK != (ec = data_add(data_ptr))) {
        data_set_next(g_data_tail, &end);
        break;
      }

      shmseg_ptr_reset(&sptr);
      *(struct shmseg_ptr_base*)&sptr = data_hdr(data_ptr)->next;
    } while (sptr.base.shm_key != -1);
  }

  g_init = (ec == E_SHM_OK);
  return ec;
}

void hamster_shutdown() {
  if (g_init) {
    g_init = false;
    rb_tree_free(g_data_tree);
    shmseg_shutdown();
  }
}

struct h_value_t* hamster_value_new(void* ptr, uint32_t size, uint32_t max_size) {
  struct h_value_t* val = NULL;

  if (ptr == NULL || size == 0 || size > max_size)
    return NULL;

  val = (struct h_value_t*)calloc(1, sizeof(struct h_value_t));
  if (val != NULL) {
    val->ptr      = ptr;
    val->size     = size;
    val->max_size = max_size;
  }
  return val;
}

struct h_value_t* hamster_value_empty() {
  return (struct h_value_t*)calloc(1, sizeof(struct h_value_t));
}

void* hamster_value_ptr(struct h_value_t* val) {
  return val ? val->ptr : NULL;
}

uint32_t hamster_value_size(struct h_value_t* val) {
  return val ? val->size : 0;
}

void hamster_value_free(struct h_value_t* val) {
  if (val) free(val);
}

// TODO: thread-safe
int hamster_set(const char* key, struct h_value_t* val) {
  int ec = E_SHM_OK;
  struct data_t stub, *target = &stub;

  if (key == NULL || val == NULL)
    return E_SHM_INVALID_PARAMS;

  stub.key = key;
  if (E_SHM_OK == (ec = rb_tree_query(g_data_tree, (void**)&target))) {
    return data_update(target, val);
  } else if (E_SHM_KEY_NOT_FOUND == ec) {
    return data_new(key, val);
  } else {
    return ec;
  }
}

// TODO: thread-safe, rw_lock may be?
int hamster_get(const char* key, struct h_value_t* val) {
  int ec;
  struct data_t stub, *target = &stub;

  if (key == NULL || val == NULL)
    return E_SHM_INVALID_PARAMS;

  stub.key = key;
  if (E_SHM_OK == (ec = rb_tree_query(g_data_tree, (void**)&target))) {
    *val = target->value;
    return E_SHM_OK;
  }
  return ec;
}

size_t hamster_count() {
  return g_data_tree->count;
}

shm_internal uint32_t hdr_key_size(struct shm_data_header* hdr) {
  return strlen((char*)(hdr + 1)) + 1;
}

shm_internal const char* hdr_key(struct shm_data_header* hdr) {
  return (const char*)(hdr + 1);
}

shm_internal uint32_t hdr_value_size(struct shm_data_header* hdr) {
  return hdr->data_size - hdr_key_size(hdr);
}

shm_internal uint32_t hdr_value_maxsize(struct shm_data_header* hdr) {
  return hdr->total_size - hdr_size - hdr_key_size(hdr);
}

shm_internal void* hdr_value(struct shm_data_header* hdr) {
  return (char*)hdr_key(hdr) + hdr_key_size(hdr);
}

shm_internal int data_load(struct data_t** d, struct shmseg_ptr* base_sptr) {
  void* base_ptr = NULL;
  struct data_t* data_ptr = NULL;
  struct shm_data_header* hdr = NULL;

  *d = NULL;
  if (NULL == (base_ptr = shmseg_ptr_ptr(base_sptr)))
    return E_SHM_PTR_INVALID;

  hdr = (struct shm_data_header*)base_ptr;
  if (hdr->checksum != data_checksum(hdr))
    return E_SHM_DATA_CORRUPTED;

  if (NULL == (data_ptr = (struct data_t*)calloc(1, sizeof(struct data_t))))
    return E_SHM_SYSTEM;

  data_ptr->base_sptr = *base_sptr;
  data_ptr->key = hdr_key(hdr);
  data_ptr->value.ptr = hdr_value(hdr);
  data_ptr->value.size = hdr_value_size(hdr);
  data_ptr->value.max_size = hdr_value_maxsize(hdr);

  *d = data_ptr;
  return E_SHM_OK;
}

shm_internal struct shm_data_header* data_hdr(struct data_t* d) {
  return (struct shm_data_header*)shmseg_ptr_ptr(&d->base_sptr);
}

shm_internal int data_checksum(struct shm_data_header* hdr) {
  return shm_crc32((char*)hdr + sizeof(int),
                   sizeof(*hdr) + hdr->data_size - sizeof(int));
}

shm_internal int data_less(void* left, void* right) {
  return strcmp(((struct data_t*)left)->key, ((struct data_t*)right)->key) < 0
         ? true : false;
}

// TODO: thread-safe
shm_internal int data_add(struct data_t* data_ptr) {
  int ec = E_SHM_OK;

  if (E_SHM_OK != (ec = rb_tree_add(g_data_tree, data_ptr))) 
    return ec;

  data_set_next(g_data_tail, (struct shmseg_ptr_base*)(&data_ptr->base_sptr));
  g_data_tail = data_ptr;
  return E_SHM_OK;
}

shm_internal int data_update(struct data_t* data_ptr, struct h_value_t* val) {
  struct shm_data_header* hdr = data_hdr(data_ptr);

  if (val->size <= data_ptr->value.max_size) {
    // copy data
    memcpy(data_ptr->value.ptr, val->ptr, val->size);
    // update header
    hdr->data_size += val->size - data_ptr->value.size;
    data_ptr->value.size = val->size;
    // update checksum
    hdr->checksum = data_checksum(hdr);
    return E_SHM_OK;
  } else {
    return E_SHM_VAL_UPDATE_EXCEED_MAX_SIZE;
  }
}

shm_internal int data_new(const char* key, struct h_value_t* val) {
  int ec = E_SHM_OK;
  void* base_ptr = NULL;
  char* data_key = NULL;
  void* data_val = NULL;
  struct data_t* data_ptr = NULL;
  uint32_t key_size = 0, total_size = 0;
  struct shmseg_ptr sptr = { { -1, 0 }, NULL };
  struct shm_data_header* hdr = NULL;

  key_size = strlen(key) + 1;
  if (key_size == 1)
    return E_SHM_KEY_ZERO_LENGTH;

  if (val->max_size < val->size)
    return E_SHM_VAL_SIZE_INVALID;

  total_size = hdr_size + key_size + val->max_size;
  if (E_SHM_OK != (ec = shmseg_get(&total_size, &sptr)))
    return ec;

  val->max_size = total_size - hdr_size - key_size;

  if (NULL == (base_ptr = shmseg_ptr_ptr(&sptr)))
    return E_SHM_PTR_INVALID;

  if (NULL == (data_ptr = (struct data_t*)calloc(1, sizeof(struct data_t))))
    return E_SHM_SYSTEM;

  data_key = (char*)base_ptr + hdr_size;
  data_val = (char*)data_key + key_size;

  // set key
  memcpy(data_key, key, key_size);
  // set value
  memcpy(data_val, val->ptr, val->size);

  hdr = (struct shm_data_header*)base_ptr;
  hdr->total_size = total_size;
  hdr->data_size = key_size + val->size;
  hdr->next.shm_key = -1;
  hdr->next.off = 0;
  hdr->checksum = data_checksum(hdr); 

  data_ptr->base_sptr = sptr;
  data_ptr->key = data_key;
  data_ptr->value = *val;
  data_ptr->value.ptr = data_val;

  return data_add(data_ptr);
}

shm_internal void data_set_next(struct data_t* data_ptr, struct shmseg_ptr_base* base_sptr) {
  struct shm_data_header* hdr = NULL;

  if (data_ptr != NULL) {
    hdr = data_hdr(data_ptr);
    hdr->next = *base_sptr;
    hdr->checksum = data_checksum(hdr);
  }
}

#ifdef UNITTEST
shm_internal void unittest_shmseg_sim_crash();
shm_internal void unittest_hamster_sim_crash() {
  rb_tree_free(g_data_tree);
  g_data_tree = NULL;
  g_data_tail = NULL;
  g_init = false;
  unittest_shmseg_sim_crash();
}
#endif

#undef hdr_size

