#ifndef MPI_DEBUG_H
#define MPI_DEBUG_H
#include "mpi/debug.h"

#include <stdio.h>
#include <unistd.h>

void gdb_attach_point(char const* const file, int line) {
    volatile int i = 0;
    while (0 == i) {
        fprintf(
            stderr,
            "PID %d waiting on " __FILE__ ":%d. Set a breakpoint on %s:%d\n",
            getpid(),
            __LINE__,
            file,
            line + 1);
        sleep(5);
    }
}

#endif // MPI_DEBUG_H
