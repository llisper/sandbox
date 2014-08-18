#ifndef worker_h
#define worker_h

#include <time.h>

struct worker_t;
typedef int (*worker_log)(const char* fmt, ...);

int worker_signal_init();

struct worker_t* worker_new(unsigned int interval, worker_log wlog);

void worker_free(struct worker_t* w);

int worker_run(struct worker_t* w);

int worker_interrupt(struct worker_t* w, time_t ap_time, int job_id);

#endif // worker_h

