#include "t_scope.h"
#include <sys/mman.h>
#include <unistd.h>
#include <stdlib.h>
#include <pthread.h>
#include <assert.h>
#include <fcntl.h>

#define INIT_ARENA_SIZE 10
#define GET_SI() struct scopeinfo_t *si = (struct scopeinfo_t*)pthread_getspecific(key)

struct scopeinfo_t {
    char **arena;
    int a_cap;
    int a_size;
    char *frame;
    char *frame_pos;
};

// use thread-local storage to save scopeinfo_t to current thread
// avoid have to specify scopeinfo ptr on parameters

struct save_ptr { char *ptr; };

static pthread_key_t key;
static pthread_once_t init_done = PTHREAD_ONCE_INIT;
static void thread_init(void);
static void scopeinfo_destroy(void*);
static int  arena_size() { return 1024 * sysconf(_SC_PAGESIZE); }
static int  arena_idx(char *frame);
static int  arena_check_expand(int size);
static void arena_shrink();

void scope_init() {
    pthread_once(&init_done, thread_init);

    struct scopeinfo_t *si = (struct scopeinfo_t*)pthread_getspecific(key);
    if (!si) {
        si = (struct scopeinfo_t*)calloc(1, sizeof(struct scopeinfo_t));
        si->arena = (char**)calloc(INIT_ARENA_SIZE, sizeof(char*));
        si->a_cap = INIT_ARENA_SIZE;
        pthread_setspecific(key, si);
    }
}

void* frame_push() {
    GET_SI();
    if (-1 == arena_check_expand((int)sizeof(char*)))
        return NULL;

    // store addr to previous frame
    char *next_frame = si->frame_pos;
    ((struct save_ptr*)next_frame)->ptr = si->frame;

    next_frame += sizeof(char*);
    si->frame_pos = si->frame = next_frame;
    return si->frame;
}

void frame_pop(void** unused) {
    GET_SI();
    if (!si->frame)
        return;

    si->frame_pos = si->frame - sizeof(char*);
    si->frame = ((struct save_ptr*)si->frame_pos)->ptr;
    arena_shrink();
}

void* t_scope_new(int size) {
    if (size > arena_size())
        return NULL;

    GET_SI();
    if (!si->frame || -1 == arena_check_expand(size))
        return NULL;

    char *ptr = si->frame_pos;
    si->frame_pos += size;
    return ptr;
}

void scope_dump() {
    GET_SI();
    if (!si) return;
    
}

static void thread_init(void) {
    pthread_key_create(&key, scopeinfo_destroy);
}

static void scopeinfo_destroy(void *ctx) {
    struct scopeinfo_t *p = (struct scopeinfo_t*)ctx;
    for (int i = 0; i < p->a_size; ++i)
        munmap(p->arena[i], arena_size());
    free(p->arena);
    free(p);
}

static int arena_idx(char *frame) {
    GET_SI();
    for (int i = 0; i < si->a_size; ++i) {
        if (frame >= si->arena[i] && frame <= si->arena[i] + arena_size())
            return i;
    }
    return -1;
}

static int arena_check_expand(int size) {
    GET_SI();
    int left = 0;
    if (si->a_size) {
        int used = (int)(si->frame_pos - si->arena[si->a_size - 1]);
        left = arena_size() - used;
    }

    if (left < size) {
        if (si->a_size == si->a_cap) 
            si->arena = (char**)realloc(si->arena, (int)(1.5 * si->a_cap));

        char *new_arena = mmap(NULL, arena_size(), 
                PROT_READ|PROT_WRITE,
                MAP_PRIVATE/*|MAP_ANON*/,
                -1, 0);

        if (new_arena == MAP_FAILED)
            return -1;

        si->arena[si->a_size++] = new_arena;
        si->frame_pos = new_arena;
    }
    return 0;
}

static void arena_shrink() {
    GET_SI();
    int idx_in_use = arena_idx(si->frame_pos);
    assert(idx_in_use >= 0);
    for (int i = idx_in_use + 1; i < si->a_size; ++i)
        munmap(si->arena[i], arena_size());
    si->a_size = idx_in_use + 1;
}

#undef INIT_ARENA_SIZE
#undef GET_SI

