#include "common/debug.h"

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdnoreturn.h>
#include <execinfo.h>

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

noreturn void debug_print_backtrace(char const* const msg) {
    eprintf("Panicked at '%s'\n", msg);
    void** func_addrs = malloc(sizeof(void*) * 30);
    int size = backtrace(func_addrs, 30);
    char const* const* const bt =
        (char const* const*) backtrace_symbols(func_addrs, size);
    eputs("Backtrace\n");
    for (char const* const* i = bt + 1; i != bt + size; ++i) {
        eprintf("%s\n", *i);
    }
    abort();
}
