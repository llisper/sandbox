#include <stdlib.h>
#include <signal.h>
#include <pthread.h>
#include <string.h>
#include <unistd.h>
#include "worker.h"

#define SIG_WORKER_INTERRUPT (SIGUSR1)
#define SIG_WORKER_KILL (SIGUSR1)
#define MAX_JOB_QUEUE 10
#define MAX_WORKERS 10

struct job_t {
    time_t ap_time;
    int id;
};

struct worker_t {
    pthread_t tid;
    unsigned int interval;
    volatile int exit_loop; // set by signal handler
    struct job_t job_q[MAX_JOB_QUEUE]; // need to protect by lock
    size_t num_job;
    pthread_mutex_t job_lock;
    worker_log wlog;
};

static void* thr_fn(void *arg);
static int null_log(const char* fmt, ...) { return 0; }
static void next_job(struct worker_t *w, struct job_t *job, unsigned int *sleep_time);
static void delete_job(struct worker_t *w, int job_id);
static void sig_handler(int signo);

int worker_signal_init() {
    struct sigaction sa;
    int ret;

    sa.sa_handler = sig_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    if (0 != (ret = sigaction(SIG_WORKER_INTERRUPT, &sa, NULL)))
        return ret;

    sa.sa_handler = sig_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    if (0 != (ret = sigaction(SIG_WORKER_KILL, &sa, NULL)))
        return ret;

    return 0;
}

struct worker_t* worker_new(unsigned int interval, worker_log wlog) {
    struct worker_t *w = (struct worker_t*)calloc(1, sizeof(struct worker_t));
    w->interval = interval;
    w->wlog = wlog ? wlog : null_log;
    pthread_mutex_init(&w->job_lock, NULL);
    return w;
}

void worker_free(struct worker_t* w) {
    int ret = pthread_kill(w->tid, SIG_WORKER_KILL);
    if (ret != 0)
        w->wlog("pthread_kill error: tid[%x], ret[%d]\n", w->tid, ret);
    pthread_join(w->tid, NULL);
    free(w);
}

int worker_run(struct worker_t* w) {
    return pthread_create(&w->tid, NULL, thr_fn, w);
}

int worker_interrupt(struct worker_t* w, time_t ap_time, int job_id) {
    size_t i; 
    pthread_mutex_lock(&w->job_lock);
    if (w->num_job >= MAX_JOB_QUEUE) {
        pthread_mutex_unlock(&w->job_lock);
        return -1;
    }

    for (i = 0; i < w->num_job; ++i)
    {
        if (ap_time >= w->job_q[i].ap_time)
        {
            memmove(&w->job_q[i + 1], &w->job_q[i], w->num_job - i);
            break;
        }
    }
    w->job_q[i].ap_time = ap_time;
    w->job_q[i].id = job_id;
    ++(w->num_job);
    pthread_mutex_unlock(&w->job_lock);

    int ret = pthread_kill(w->tid, SIG_WORKER_INTERRUPT);
    if (ret != 0)
        w->wlog("pthread_kill error: tid[%x], ret[%d]\n", w->tid, ret);

    return 0;
}

static void next_job(struct worker_t *w, struct job_t *job, unsigned int *sleep_time)
{
    time_t now = time(NULL);
    struct job_t *j;

    job->ap_time = 0;
    job->id = -1;
    if (w->num_job > 0) {
        pthread_mutex_lock(&w->job_lock);
        if (w->num_job > 0) {
            j = &w->job_q[w->num_job - 1]; 
            if (now + *sleep_time >= j->ap_time)
            {
                *job = *j;
                if (job->ap_time > now)
                    *sleep_time = (unsigned int)(job->ap_time - now);
                else 
                    *sleep_time = 0;
            }
        }
        pthread_mutex_unlock(&w->job_lock);
    }
}

static void delete_job(struct worker_t *w, int job_id) {
    size_t i;
    pthread_mutex_lock(&w->job_lock);
    for (i = 0; i < w->num_job; ++i) {
        if (w->job_q[i].id == job_id) {
            memcpy(&w->job_q[i], &w->job_q[i + 1], w->num_job - i);
            --(w->num_job);
            break;
        }
    }
    pthread_mutex_unlock(&w->job_lock);
}

static void* thr_fn(void *arg) {
    struct worker_t *w = (struct worker_t*)arg;
    struct job_t job;
    unsigned int sleep_time = 0, slept_time;
    time_t start, now;
    char now_str[128], ap_str[128];
    struct tm now_tm, ap_tm;
    sigset_t mask;

    sigfillset(&mask);
    sigdelset(&mask, SIG_WORKER_INTERRUPT);
    sigdelset(&mask, SIG_WORKER_KILL);
    pthread_sigmask(SIG_BLOCK, &mask, NULL);

    while (!__sync_fetch_and_add(&w->exit_loop, 0)) {
        start = time(NULL);
        do {
            if (sleep_time == 0)
                sleep_time = w->interval;
            next_job(w, &job, &sleep_time);
            if (job.id > 0)
                w->wlog("next job[%d] start in %d seconds\n", job.id, sleep_time);
            sleep_time = sleep(sleep_time);
            if (sleep_time > 0)
                w->wlog("interrupt by signal, %d seconds left\n", sleep_time);
        }
        while (sleep_time > 0);

        now = time(NULL);
        slept_time = now - start;
        strftime(now_str, 128, "%T", localtime_r(&now, &now_tm));
        if (job.id > 0) {
            strftime(ap_str, 128, "%T", localtime_r(&job.ap_time, &ap_tm));
            w->wlog("[%s] do job[%d], appointment time[%s], slept %d seconds\n",
                    now_str, job.id, ap_str, slept_time);
            delete_job(w, job.id);
        } else {
            w->wlog("[%s] slept %d seconds\n", now_str, slept_time);
        }
    }

    return NULL;
}

static void sig_handler(int signo) {
    if (signo == SIG_WORKER_KILL) {
        // do nothing for now            
    }
}

