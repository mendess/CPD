#ifndef MPI_DEBUG_H
#define MPI_DEBUG_H

#include <stdnoreturn.h>

#define GDB_ATTACH_POINT (gdb_attach_point(__FILE__, __LINE__))

void gdb_attach_point();

noreturn void debug_print_backtrace(char const*);

#ifdef MPI
extern int G_ME;
#    define eprintf(fmt, ...) \
        (fprintf(stderr, "Node %d: " fmt, G_ME, __VA_ARGS__),fflush(stderr))
#else
#    define G_ME 0
#    define eprintf(...) (fprintf(stderr, __VA_ARGS__),fflush(stderr))
#endif // MPI
#define eprintln(fmt, ...) eprintf(fmt "\n", __VA_ARGS__)
#define eputs(s) eprintf("%s", (s))
#define eputln(s) eprintln("%s", (s))

#endif // MPI_DEBUG_H
