#ifndef MPI_DEBUG_H
#define MPI_DEBUG_H

#define GDB_ATTACH_POINT (gdb_attach_point(__FILE__, __LINE__))

void gdb_attach_point();

#endif // MPI_DEBUG_H
