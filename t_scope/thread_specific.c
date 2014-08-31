#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static void destructor(void*);
static void* thr_func(void*);
static pthread_once_t init_done = PTHREAD_ONCE_INIT;
static void thread_init(void);
static pthread_key_t key;

int main(int argc, char *argv[]) {
    int retcode;
    pthread_t tid[2];

    for (int i = 0; i < 2; ++i) {
        if (0 != (retcode = pthread_create(&tid[i], NULL, thr_func, NULL)))
            return retcode;
    }

    for (int i = 0; i < 2; ++i) 
        pthread_join(tid[i], NULL);

    return 0;
}

static void* thr_func(void* ctx) {
    pthread_once(&init_done, thread_init);

    void *data = pthread_getspecific(key);
    if (!data) {
        data = calloc(1, 64);
        pthread_setspecific(key, data);
    }

    snprintf(data, 64, "hello world from [%0x]!\n", (unsigned)pthread_self());
    sleep(1);
    return NULL;
}

static void destructor(void* data) {
    puts((char*)data);
    free(data);
}

static void thread_init(void) {
    pthread_key_create(&key, destructor);
}

