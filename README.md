# CPD

High performance matrix processing, see [./assginment.pdf](assignment.pdf) to understand the math

This project implements the same algorithm in three ways, sequential (`serial`), parallel with `openmp` and parallel with `mpi`.

## Compiling

```
make
```

Executable files will be in `target/{debug,release}/{serial,openmp,mpi}/recomender`

## Running automated tests

```
./run_tests.sh
```
