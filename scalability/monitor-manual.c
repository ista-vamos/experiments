#include <assert.h>
#include <fcntl.h>
#include <immintrin.h> /* _mm_pause */
#include <limits.h>
#include <stdio.h>
#include <string.h>
#include <threads.h>
#include <time.h>
#include <unistd.h>

#include "arbiter.h"
#include "event.h"
#include "shamon.h"
#include "signatures.h"
#include "source.h"
#include "streams/stream-generic.h"
#include "utils.h"
#include "vector.h"

vms_stream *create_stream(int argc, char *argv[], int arg_i,
                          const char *expected_stream_name);

static int stream_running;

static int buffer_thrd(void *data) {
    vms_arbiter_buffer           *buffer = (vms_arbiter_buffer *)data;
    vms_stream                   *stream = vms_arbiter_buffer_stream(buffer);
    register vms_stream_alter_fn  alter  = stream->alter;
    register vms_stream_filter_fn filter = stream->filter;
    const size_t                  size   = stream->event_size;

    // wait for buffer->active
    while (!vms_arbiter_buffer_active(buffer))
        _mm_pause();

    printf("Running fetch & autodrop for stream %s\n", stream->name);

    void *ev, *out;
    while (1) {
        ev = vms_stream_fetch(stream, buffer);
        if (!ev) {
            break;
        }

        out = vms_arbiter_buffer_write_ptr(buffer);
        assert(out && "No space in the buffer");
        memcpy(out, ev, size);
        vms_arbiter_buffer_write_finish(buffer);
        vms_stream_consume(stream, 1);
    }

    // TODO: we should check if the stream is finished and remove it
    // in that case
    printf("BMM for stream %lu (%s) exits\n", stream->id, stream->name);
    stream_running = 0;
    thrd_exit(EXIT_SUCCESS);
}

int main(int argc, char *argv[]) {
    if (argc < 3 || argc > 5) {
        fprintf(stderr, "USAGE: prog name:source:shmkey buffer-capacity "
                        "[wait-cycles] [max-consume]\n");
        return -1;
    }

    size_t wait_cycles = 0;
    size_t max_consume = ~0UL;
    size_t capacity    = atoll(argv[2]);
    if (argc >= 4)
        wait_cycles = atoll(argv[3]);
    if (argc >= 5)
        max_consume = atoll(argv[4]);

    printf("Buffer capacity: %lu, wait %lu cycles after each event, "
           "consume at most %lu events at once\n",
           capacity, wait_cycles, max_consume);

    initialize_events();

    vms_stream         *stream = create_stream(argc, argv, 1, NULL);
    vms_arbiter_buffer *buffer = vms_arbiter_buffer_create(
        stream, sizeof(vms_event) + sizeof(size_t), capacity);

    thrd_t tid;
    thrd_create(&tid, buffer_thrd, buffer);
    stream_running = 1;
    vms_arbiter_buffer_set_active(buffer, 1);

    size_t          n = 0, tmp, trials = 0;
    volatile size_t cycles;
    while (1) {
        tmp = vms_arbiter_buffer_size(buffer);
        if (tmp > 0) {
            n += vms_arbiter_buffer_drop(buffer,
                                         tmp > max_consume ? max_consume : tmp);
        }

        if (tmp == 0 && !stream_running)
            break;

        cycles = 0;
        while (cycles < wait_cycles)
            ++cycles;
    }
    printf("Processed %lu events\n", n);
    printf("Dropped %lu times in total %lu events\n",
           vms_arbiter_buffer_dropped_times(buffer),
           vms_arbiter_buffer_dropped_num(buffer));
#if DUMP_STATS
    vms_arbiter_buffer_dump_stats(buffer);
#endif

    thrd_join(tid, 0);
    vms_stream_destroy(stream);
    deinitialize_events();
}
