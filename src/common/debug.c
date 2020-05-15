#include "common/debug.h"
#define _POSIX_C_SOURCE 200112L

#include <execinfo.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdnoreturn.h>
#include <sys/wait.h>
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

static bool print_trace() {
    char pid_buf[30];
    sprintf(pid_buf, "%d", getpid());
    char name_buf[512];
    name_buf[readlink("/proc/self/exe", name_buf, 511)] = 0;
    int child_pid = fork();
    if (!child_pid) {
        eprintln("Stack trace for %s pid=%s", name_buf, pid_buf);
        dup2(2, 1);
        execlp(
            "gdb",
            "gdb",
            "--batch",
            "-n",
            "-ex",
            "thread",
            "-ex",
            getenv("BT_FULL") ? "bt full" : "bt",
            name_buf,
            pid_buf,
            NULL);
        perror("Gdb failed to start");
        _exit(1);
    } else {
        int status;
        waitpid(child_pid, &status, 0);
        return WIFEXITED(status) && WEXITSTATUS(status) == 0;
    }
}

noreturn void debug_print_backtrace(char const* const msg) {
    eprintf("Panicked at '%s'\n", msg);
    if (!print_trace()) {
        void** func_addrs = malloc(sizeof(void*) * 30);
        int size = backtrace(func_addrs, 30);
        char const* const* const bt =
            (char const* const*) backtrace_symbols(func_addrs, size);
        for (char const* const* i = bt + 1; i != bt + size; ++i) {
            eprintf("%s\n", *i);
        }
    }
    abort();
}
