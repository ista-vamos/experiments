#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#ifdef SHM_DOMONITOR_WAIT
#define SHM_DOMONITOR
#include "../../fastbuf/vms_monitored.h"
#define SHM_MONFUN(...) push_data_wait(__VA_ARGS__)
#endif
#ifdef SHM_DOMONITOR_NOWAIT
#define SHM_DOMONITOR
#include "../../fastbuf/vms_monitored.h"
#define SHM_MONFUN(...) push_data_nowait(__VA_ARGS__)
#endif

static int hash(char *str) {
    int h = 2345607;
    int l = strlen(str);
    for (int i = l - 1; i >= 0; i--) {
        int c = (int)(str[i]);
        h     = ((h + c) * 65537) % 114460513;
    }
    return h;
}

static char *linebuf[16] = {NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
                            NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL};
static int   linebufpos  = 0;

static void resetLinebuf() {
    for (int i = linebufpos; i > 0;) {
        i--;
        free(linebuf[i]);
        linebuf[i] = NULL;
    }
}

static char *getLine() {
    printf(">");
    fflush(stdout);
    if (linebufpos >= 16) {
        return NULL;
    }
    size_t  zero     = 0;
    ssize_t lineSize = getline(&linebuf[linebufpos], &zero, stdin);
#ifdef SHM_DOMONITOR
    char readbuf[1024];
    *((int *)readbuf) = STDIN_FILENO;
    memcpy(readbuf + 4, linebuf[linebufpos], lineSize + 1);
    SHM_MONFUN(2, readbuf, lineSize + 5);
#endif
    if (lineSize >= 0) {
        return linebuf[linebufpos];
    }
    return NULL;
}

static void removeNewLineAtEnd(char *str) {
    if (str[strlen(str) - 1] == '\n') {
        str[strlen(str) - 1] = 0;
    }
}
static int strsplit(char *str, char delim, int count) {
    int actual = 1;
    while (count > 0 && strlen(str) > 0) {
        char *next = strchr(str, delim);
        if (next != NULL) {
            next[0] = 0;
            str     = next + 1;
            count--;
            actual++;
        } else {
            count = 0;
        }
    }
    return actual;
}

#ifdef SHM_DOMONITOR
#define mprint(...)                                                            \
    {                                                                          \
        char cmn_printbuf[2048];                                               \
        sprintf(cmn_printbuf + 4, __VA_ARGS__);                                \
        int cmn_wlen             = strlen(cmn_printbuf + 4);                   \
        *((int *)(cmn_printbuf)) = STDOUT_FILENO;                              \
        SHM_MONFUN(1, cmn_printbuf, cmn_wlen + 5);                             \
        write(STDOUT_FILENO, cmn_printbuf + 4, cmn_wlen);                      \
    }
#else
#define mprint(...) printf(__VA_ARGS__)
#endif
