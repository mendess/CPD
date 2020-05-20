#ifndef DEBUG_H
#define DEBUG_H

#include <stdio.h>
#include <stdnoreturn.h>

#define GDB_ATTACH_POINT (gdb_attach_point(__FILE__, __LINE__))

void gdb_attach_point(char const* const file, int line);

noreturn void debug_print_backtrace(char const*);

#ifdef MPI
extern unsigned G_ME;
#    define eprint(fmt, ...) \
        (fprintf(stderr, "Node %d: " fmt, G_ME, __VA_ARGS__), fflush(stderr))
#else
#    define eprint(...) (fprintf(stderr, __VA_ARGS__), fflush(stderr))
#endif // MPI
#define eprintln(fmt, ...) eprint(fmt "\n", __VA_ARGS__)
#define eputs(s) eprint("%s", (s))
#define eputln(s) eprintln("%s", (s))

#endif // DEBUG_H
