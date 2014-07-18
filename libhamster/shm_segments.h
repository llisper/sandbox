#ifndef SHM_SEGMENTS_H
#define SHM_SEGMENTS_H

#include <stdlib.h>
#include <stdint.h>

/*
 * share memory segments management:
 * 1. manage segment allocation and reload from a crash
 * 2. ensure enough size for client, but do not care or manage the content
 * 3. TODO: check if a ptr is within valid range
 */

struct shmseg_ptr_base {
  key_t    shm_key;
  uint32_t off; 
};

struct shmseg_ptr {
  struct shmseg_ptr_base base;
  void*                  cache_ptr;
};

/*
 * initialise the shmseg, if client is safely shutdown last time, all shm 
 * should be delete, if they remain attachable means client was suffering a
 * crash and try to recovery, in that case, we reattach all shm 
 */
int shmseg_init(key_t entry_key);

/*
 * shutdown, delete all shm
 */
void shmseg_shutdown();

/*
 * ensure size bytes are available, allocate new shm if necessary
 */
int shmseg_get(uint32_t* size, struct shmseg_ptr* sptr); 

/*
 * get the first shmseg_ptr of shm chain
 */
int shmseg_first_ptr(struct shmseg_ptr* sptr);

/*
 * get ptr of shmseg_ptr
 */
void* shmseg_ptr_ptr(struct shmseg_ptr* sptr);

/*
 * reset the content of shmseg_ptr
 */
void shmseg_ptr_reset(struct shmseg_ptr* sptr);

#endif // SHM_SEGMENTS_H

