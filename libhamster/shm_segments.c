#include <string.h>
#include <unistd.h>
#include <sys/shm.h>

#include "shm_error.h"
#include "shm_config.h"
#include "shm_segments.h"

struct seg_t {
  key_t   shm_key;    /* shm key */
  int     shm_id;     /* shm_id after successfully attach to shm with shm_key */
  size_t  seg_size;   /* segment size */
  void*   base_ptr;   /* ptr to the begining of the segment after successfully attached */
  struct  seg_t* next;/* next seg_t */
};

/* 
 * a 16 bytes header for each shm segment 
 * why 16 bytes ? beacuse the size of key_t is undetermined under different 
 * system, some might define it as int, and some might define it as long
 */
struct seg_header {
  uint32_t off;          /* offset of used part */
  key_t    next_shm_key; /* next shm segment */
} __attribute__((aligned(16)));

shm_internal key_t  g_entry_key; 
shm_internal key_t  g_last_key;
shm_internal struct seg_t* g_seg_head;
shm_internal struct seg_t* g_seg_cur;
shm_internal struct seg_t* g_seg_tail;

shm_internal struct seg_header* seg_hdr(struct seg_t* s);
shm_internal struct seg_t*      seg_new(key_t key, size_t size_hint, bool attach_only);

shm_internal uint32_t seg_off(struct seg_t* s);
shm_internal void     seg_consume(struct seg_t* s, uint32_t off);
shm_internal size_t   seg_available_size(struct seg_t* s);
shm_internal key_t    seg_next_shm_key(struct seg_t* s);
shm_internal bool     seg_empty(struct seg_t* s);
shm_internal int      seg_add(struct seg_t* s);

int shmseg_init(key_t entry_key) {
  int ec = E_SHM_OK;
  struct seg_t *s = NULL, *next = NULL;
  key_t next_shm_key = -1;

  g_entry_key = entry_key;
  g_last_key = g_entry_key;
  if ((s = seg_new(entry_key, 0, false)) == NULL)
    return E_SHM_CREAT_SEGINFO_FAILED;

  ec = seg_add(s);
  if (E_SHM_OK != ec)
    return ec;

  g_seg_cur = s;
  next_shm_key = seg_next_shm_key(s);
  while (-1 != next_shm_key) {
    if ((next = seg_new(next_shm_key, 0, true)) == NULL) {
      seg_hdr(s)->next_shm_key = -1;
      break;
    }

    s = next;
    ec = seg_add(s);
    if (E_SHM_OK != ec)
      return ec;

    if (!seg_empty(s))
      g_seg_cur = s;

    g_last_key = next_shm_key;
    next_shm_key = seg_next_shm_key(s);
  }

  return E_SHM_OK;
}

void shmseg_shutdown() {
  struct seg_t* s = g_seg_head;
  while (s != NULL) {
    if (0 != shmdt(s->base_ptr) ||
        0 != shmctl(s->shm_id, IPC_RMID, NULL)) {
      abort();
    }
    g_seg_head = s->next;
    free(s);
    s = g_seg_head;
  }
}

// TODO: thread-safe
int shmseg_get(uint32_t* size, struct shmseg_ptr* sptr) {
  int i = 1;
  int ec = E_SHM_OK;
  uint32_t actual_size = ((*size + 15) >> 4) << 4; // round size to 16
  key_t next_shm_key = -1;
  struct seg_t* s = NULL;
  
  // 2. from g_seg_cur, check if there's enough space for this alloc
  //    if it does: adjust id/off and return 
  //    if it doesn't: alloc a new segment and return the id/off of next seg
  if (seg_available_size(g_seg_cur) < actual_size) {
    for (i = 1; s == NULL && i <= SHM_KEY_RETRY; ++i) {
      next_shm_key = g_last_key + i;    
      if (next_shm_key != -1) {
        s = seg_new(next_shm_key,
                    actual_size + sizeof(struct seg_header),
                    false);
      }
    }

    if (s == NULL)
      return E_SHM_CREAT_SEGINFO_FAILED;

    ec = seg_add(s);
    if (E_SHM_OK != ec)
      return ec;

    g_last_key = next_shm_key;
    g_seg_cur = s;
  }

  sptr->base.shm_key = g_seg_cur->shm_key;
  sptr->base.off = seg_off(g_seg_cur);
  sptr->cache_ptr = g_seg_cur->base_ptr + sptr->base.off;

  seg_consume(g_seg_cur, actual_size);
  *size = actual_size;
  return E_SHM_OK;
}

// TODO: thread-safe
int shmseg_first_ptr(struct shmseg_ptr* sptr) {
  if (!seg_empty(g_seg_head)) {
    sptr->base.shm_key = g_seg_head->shm_key;
    sptr->base.off = sizeof(struct seg_header);
    sptr->cache_ptr = shmseg_ptr_ptr(sptr);
    return E_SHM_OK;
  }
  return E_SHM_EMPTY;
}

// TODO: thread-safe
void* shmseg_ptr_ptr(struct shmseg_ptr* sptr) {
  struct seg_t* s = g_seg_head;
  if (sptr->cache_ptr == NULL) {
    for (; s != NULL; s = s->next) {
      if (s->shm_key == sptr->base.shm_key) {
        sptr->cache_ptr = sptr->base.off < s->seg_size 
          ? s->base_ptr + sptr->base.off 
          : NULL;
      }
    }
  }
  return sptr->cache_ptr;
}

void shmseg_ptr_reset(struct shmseg_ptr* sptr) {
  memset(sptr, 0, sizeof(struct shmseg_ptr));
  sptr->base.shm_key = -1;
}

shm_internal uint32_t seg_off(struct seg_t* s) {
  return seg_hdr(s)->off;
}

shm_internal void seg_consume(struct seg_t* s, uint32_t size) {
  seg_hdr(s)->off += size;
}

shm_internal size_t seg_available_size(struct seg_t* s) {
  return s->seg_size - seg_hdr(s)->off;
}

shm_internal key_t seg_next_shm_key(struct seg_t* s) {
  return seg_hdr(s)->next_shm_key;
}

shm_internal bool seg_empty(struct seg_t* s) {
  return seg_hdr(s)->off == sizeof(struct seg_header);
}

shm_internal struct seg_header* seg_hdr(struct seg_t* s) {
  return (struct seg_header*)s->base_ptr;
}

/* TODO: thread-safe */
shm_internal struct seg_t* seg_new(key_t key, size_t size_hint, bool attach_only) {
  int    shm_id = -1;
  long   shm_size = 0;
  struct seg_t* s = NULL;
  void*  base_ptr = NULL;
  struct shmid_ds buf;
  struct seg_header* h = NULL;
  char   zero_header[sizeof(struct seg_header)] = { 0 };

  if ((shm_id = shmget(key, 0, 0600)) < 0) {
    if (attach_only)
      return NULL;

    shm_size = shm_pagesize * SHM_SIZE_IN_PAGES;
    if (shm_size < size_hint) 
      shm_size = ((size_hint + shm_pagesize - 1) / shm_pagesize) * shm_pagesize;

    if ((shm_id = shmget(key, shm_size, 0600 | IPC_CREAT)) < 0)
      return NULL;
  }

  base_ptr = shmat(shm_id, 0, 0);
  if (base_ptr == (void*)-1)
    return NULL;

  if (shmctl(shm_id, IPC_STAT, &buf) < 0)
    return NULL;

  if ((s = malloc(sizeof(struct seg_t))) == NULL)
    return NULL;

  s->shm_key  = key;
  s->shm_id   = shm_id;
  s->seg_size = buf.shm_segsz;
  s->base_ptr = base_ptr;
  s->next     = NULL;

  /* read header */
  h = seg_hdr(s);
  if (0 == memcmp(h, zero_header, sizeof(struct seg_header))) {
    /* header is empty */
    h->off = sizeof(struct seg_header);
    h->next_shm_key = -1;
  }

  return s;
}

/* TODO: thread-safe */
shm_internal int seg_add(struct seg_t* s) {
  if (g_seg_head == NULL) {
    g_seg_head = g_seg_tail = s;
  } else {
    g_seg_tail->next = s;
    seg_hdr(g_seg_tail)->next_shm_key = s->shm_key;
    g_seg_tail = s;
  }

  return E_SHM_OK;
}

/* unittest call only */
shm_internal void unittest_shmseg_sim_crash() {
  struct seg_t* s = g_seg_head;
  while (s != NULL) {
    shmdt(s->base_ptr);
    g_seg_head = s->next;
    free(s);
    s = g_seg_head;
  }

  g_seg_head = g_seg_cur = g_seg_tail = NULL;
  g_entry_key = g_last_key = 0;
}

shm_internal struct seg_t* unittest_seg_head() {
  return g_seg_head;
}

shm_internal struct seg_t* unittest_seg_next(struct seg_t* s) {
  return s->next;
}

shm_internal void* unittest_seg_base(struct seg_t* s) {
  return s->base_ptr + sizeof(struct seg_header);
}

