#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "vamos-buffers/core/signatures.h"
#include "vamos-buffers/core/source.h"
#include "vamos-buffers/core/event.h"
#include "vamos-buffers/shmbuf/buffer.h"
#include "vamos-buffers/shmbuf/client.h"

static void usage_and_exit(int ret) {
    fprintf(stderr, "Usage: ./source shmkey shm-buffer-capacity freq [num of events]\n");
    exit(ret);
}

// #define WITH_LINES
// #define WITH_STDOUT

#ifndef WITH_STDOUT
#define printf(...)                                                            \
    do {                                                                       \
    } while (0)
#endif

static size_t waiting_for_buffer = 0;

int main(int argc, char *argv[]) {
    size_t events_num = ~0UL;
    size_t freq = 0;
    if (argc < 4 || argc > 5) {
        usage_and_exit(1);
    }

    const char *shmkey = argv[1];
    size_t shm_buffer_capacity = atoll(argv[2]);

    if (argc >= 4) {
        freq = atoll(argv[3]);
        if (argc == 5) {
            events_num = atoll(argv[4]);
        } else if (argc != 4) {
            usage_and_exit(1);
        }
    } else {
        usage_and_exit(1);
    }

    /* Initialize the info about this source */
    struct vms_source_control *control = vms_source_control_define(1, "E", "l");
    assert(control);
    vms_shm_buffer *shm = vms_shm_buffer_create(shmkey, shm_buffer_capacity, control);
    assert(shm);
    free(control);
    fprintf(stderr, "info: waiting for the monitor to attach... ");
    vms_shm_buffer_wait_for_reader(shm);
    fprintf(stderr, "done\n");

    size_t               num;
    struct vms_event_record *events = vms_shm_buffer_get_avail_events(shm, &num);
    assert(num == 1);

    vms_event base = {.id = 0, .kind = events[0].kind};
    assert(base.kind != 0 && "Monitor did not set kind");

    void           *addr;
    size_t          n = 0;
    volatile size_t cycles;
    while (++n <= events_num) {
        while (!(addr = vms_shm_buffer_start_push(shm))) {
            ++waiting_for_buffer;
        }
        /* push the base info about event */
        ++base.id;
        addr = vms_shm_buffer_partial_push(shm, addr, &base, sizeof(base));
        vms_shm_buffer_partial_push(shm, addr, &n, sizeof(n));
        vms_shm_buffer_finish_push(shm);

        /* wait `freq` cycles before sending next event */
        cycles = 0;
        while (cycles < freq)
            ++cycles;
    }

    /* Free up memory held within the regex memory */
    fprintf(stderr, "info: sent %lu events, busy waited on buffer %lu cycles\n",
            base.id, waiting_for_buffer);
    vms_shm_buffer_destroy(shm);

    return 0;
}
