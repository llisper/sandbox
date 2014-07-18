#ifndef LIBHAMSTER_H
#define LIBHAMSTER_H

#include <stdlib.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

struct h_value_t;

/*
 * initialise data, call after shmseg_init
 */
int hamster_init();

/*
 * release all resources
 */
void hamster_shutdown();

/*
 * create a value type
 */
struct h_value_t* hamster_value_new(void* ptr, uint32_t size, uint32_t max_size);

/*
 * create a empty value
 */
struct h_value_t* hamster_value_empty();

/*
 * get h_value_t.ptr
 */
void* hamster_value_ptr(struct h_value_t* val);

/*
 * get h_value_t.size
 */
uint32_t hamster_value_size(struct h_value_t* val);

/*
 * destroy a value type
 */
void hamster_value_free(struct h_value_t* val);

/*
 * set value to key
 * if the key do not exist, create a new one in shm
 * else the size of val must be less or equal to the max_size of val which 
 * determined when this key-val is first set.
 *
 * val->max_size might be enlarge a little due to the 16byte-boundary constaint
 */
int hamster_set(const char* key, struct h_value_t* val);

/*
 * get value by key
 */
int hamster_get(const char* key, struct h_value_t* val);

/*
 * get cache count
 */
size_t hamster_count();

#ifdef __cplusplus
}
#endif

#endif // LIBHAMSTER_H

