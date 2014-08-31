#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "worker.h"

int main(int argc, char *argv[]) {
    struct worker_t* w;
    unsigned int interval;
    int job_id_cnt = 1;
    char input[128];
    time_t ap_secs;

    if (argc != 2) {
        printf("%s [interval]\n", argv[0]);
        return 1;
    }

    interval = (unsigned int)atoi(argv[1]);

    assert(0 == worker_signal_init());
    assert(NULL != (w = worker_new(interval, printf)));
    assert(0 == worker_run(w));

    while (1) {
        scanf("%s", input);
        if (0 == strcmp(input, "quit"))
            break;
        ap_secs = (time_t)atoi(input);
        assert(0 == worker_interrupt(w, time(NULL) + ap_secs, job_id_cnt++));
    }

    return 0; 
}

