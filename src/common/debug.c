#ifndef MPI_DEBUG_H
#define MPI_DEBUG_H
#include "common/debug.h"

#include <stdio.h>
#include <unistd.h>

void gdb_attach_point(char const* const file, int line) {
    volatile int i = 0;
    fprintf(
        stderr,
        "Run these commands to attach:\n"
        "gdb --pid %d\n"
        "b " __FILE__ ":%d\n"
        "c\n"
        "set var i = 42\n"
        "b %s:%d\n"
        "c\n",
        getpid(),
        __LINE__ + 4,
        file,
        line + 1);
    while (0 == i) {
        sleep(5);
    }
}

#endif // MPI_DEBUG_H
