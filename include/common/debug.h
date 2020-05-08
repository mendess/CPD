#ifndef MPI_DEBUG_H
#define MPI_DEBUG_H

#define GDB_ATTACH_POINT (gdb_attach_point(__FILE__, __LINE__))

void gdb_attach_point();


#ifdef MPI
extern int G_ME;
#define eprintf(fmt, ...) (fprintf(stderr, "Node %d: " fmt, G_ME, ##__VA_ARGS__))
#else
#define G_ME 0
#define eprintf(...) (fprintf(stderr, ##__VA_ARGS__))
#endif // MPI

#endif // MPI_DEBUG_H
