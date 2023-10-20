#include "drfun/events.h"
#include "shmbuf/monitor.h"
#include "shmbuf/shm.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

int isprime(int);

int main(void) {
    vms_shm_buffer *shm = get_shared_buffer();
    assert(shm && "Failed getting shared buffer");
    vms_shm_buffer *local = get_local_buffer(shm);
    if (!local) {
        release_shared_buffer(shm);
        fprintf(stderr, "Out of memory");
        return -1;
    }

    buffer_register_sync_monitor(shm);
    unsigned char *data = buffer_get_beginning(local);
    union IT {
        size_t        *fun_off;
        unsigned      *i;
        unsigned char *raw;
    } ptr;
    unsigned short       n = 0;
    struct call_event_1i ev;
    while (buffer_is_ready(shm)) {
        size_t size = buffer_read(shm, local);
        printf("read %lu bytes\n", size);
        ptr.raw = data;
        for (; ptr.raw - data < size;) {
            if (n == 0) {
                ev.addr = *ptr.fun_off;
                ++ptr.fun_off;
                n += sizeof(size_t);
            } else {
                assert(n == sizeof(size_t));
                ev.argument = *ptr.i;
                ++ptr.i;
                n = 0;

                // we've got all
                printf("fun: %lu\n", ev.addr);
                printf("arg0: %u\n", ev.argument);
                if (!isprime(ev.argument)) {
                    printf("Not a prime: %u\n", ev.argument);
                }
            }
        }
    }

    // printf("Number of primes: %lu\n", n);

    free(local);
    // FIXME: we do not have the right filedescr.
    // release_shared_buffer(shm);
}
