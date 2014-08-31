#include <pthread.h>
#include <stdio.h>
#include <signal.h>
#include <unistd.h>

volatile int thr_ready = 0;
pthread_mutex_t thr_init_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t thr_init = PTHREAD_COND_INITIALIZER;
void sig_handler(int signo);

void* thr_fn(void *arg) {
    int signo;
    sigset_t mask, wait;

    sigfillset(&mask);
    sigdelset(&mask, SIGINT);
    pthread_sigmask(SIG_BLOCK, &mask, NULL);

    pthread_mutex_lock(&thr_init_lock);
    thr_ready = 1;
    pthread_mutex_unlock(&thr_init_lock);
    pthread_cond_signal(&thr_init);

    printf("thread[%x] waiting for SIGINT\n", (int)pthread_self());
    sleep(999);
    /*
    sigemptyset(&wait);
    sigaddset(&wait, SIGINT);
    if (sigwait(&wait, &signo) < 0)
        return (void*)4;
        */

    printf("thread[%x] stop waiting for SIGINT\n", (int)pthread_self());
    return NULL;
}

void sig_handler(int signo)
{
    printf("caught sig[%d] in thread[%x]\n", signo, (int)pthread_self());
}

typedef void (*worker_log)(const char* fmt, ...);

int main(void) {
    int ret;
    void *thr_ret;
    pthread_t tid;
    struct sigaction sa;
    sa.sa_handler = sig_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    if (sigaction(SIGINT, &sa, NULL) < 0)
        return 1;

    if (0 != (ret = pthread_create(&tid, NULL, thr_fn, 0)))
        return 2;

    printf("pid: %d\nmain: [%x]\nthr: [%x]\n", (int)getpid(), (int)pthread_self(), (int)tid);

    pthread_mutex_lock(&thr_init_lock);
    while (!thr_ready)
        pthread_cond_wait(&thr_init, &thr_init_lock);
    pthread_mutex_unlock(&thr_init_lock);

    printf("send SIGINT to thread[%x]\n", (int)tid);
    ret = pthread_kill(tid, SIGINT);
    printf("pthread_kill return %d\n", ret);
    if (ret != 0)
        return 3;

    pthread_join(tid, &thr_ret);
    printf("thread exit: %d\n", (int)thr_ret);

    worker_log wl = printf;
    wl("wl\n");

    return 0;
}

