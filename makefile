SOURCES_DIR = src
BUILD_DIR  = target
HEADERS = include
HEADERS_COMMON = include/common
HEADERS_SERIAL = include/serial
HEADERS_OPENMP = include/openmp
HEADERS_MPI    = include/mpi

DEBUG_DIR_COMMON = $(BUILD_DIR)/debug/common
DEBUG_DIR_SERIAL = $(BUILD_DIR)/debug/serial
DEBUG_DIR_OPENMP = $(BUILD_DIR)/debug/openmp
DEBUG_DIR_MPI    = $(BUILD_DIR)/debug/mpi
DEBUG_DIR_COMMON_MPI = $(BUILD_DIR)/debug/mpi/common

RELEASE_DIR_COMMON = $(BUILD_DIR)/release/common
RELEASE_DIR_SERIAL = $(BUILD_DIR)/release/serial
RELEASE_DIR_OPENMP = $(BUILD_DIR)/release/openmp
RELEASE_DIR_MPI    = $(BUILD_DIR)/release/mpi
RELEASE_DIR_COMMON_MPI = $(BUILD_DIR)/release/mpi/common

SOURCES_COMMON_DIR = $(SOURCES_DIR)/common
SOURCES_SERIAL_DIR = $(SOURCES_DIR)/serial
SOURCES_OPENMP_DIR = $(SOURCES_DIR)/openmp
SOURCES_MPI_DIR    = $(SOURCES_DIR)/mpi

SOURCES_COMMON = $(wildcard $(SOURCES_COMMON_DIR)/*.c)
SOURCES_SERIAL = $(wildcard $(SOURCES_SERIAL_DIR)/*.c)
SOURCES_OPENMP = $(wildcard $(SOURCES_OPENMP_DIR)/*.c)
SOURCES_MPI    = $(wildcard $(SOURCES_MPI_DIR)/*.c)

OBJ_DEBUG_COMMON = $(foreach o, $(patsubst $(SOURCES_COMMON_DIR)/%.c, %.o, $(SOURCES_COMMON)), $(DEBUG_DIR_COMMON)/$o)
OBJ_DEBUG_COMMON_MPI = $(foreach o, $(patsubst $(SOURCES_COMMON_DIR)/%.c, %.o, $(SOURCES_COMMON)), $(DEBUG_DIR_COMMON_MPI)/$o)
OBJ_DEBUG_SERIAL = $(foreach o, $(patsubst $(SOURCES_SERIAL_DIR)/%.c, %.o, $(SOURCES_SERIAL)), $(DEBUG_DIR_SERIAL)/$o)
OBJ_DEBUG_OPENMP = $(foreach o, $(patsubst $(SOURCES_OPENMP_DIR)/%.c, %.o, $(SOURCES_OPENMP)), $(DEBUG_DIR_OPENMP)/$o)
OBJ_DEBUG_MPI    = $(foreach o, $(patsubst $(SOURCES_MPI_DIR)/%.c, %.o, $(SOURCES_MPI)), $(DEBUG_DIR_MPI)/$o)

OBJ_RELEASE_COMMON = $(foreach o, $(patsubst $(SOURCES_COMMON_DIR)/%.c, %.o, $(SOURCES_COMMON)), $(RELEASE_DIR_COMMON)/$o)
OBJ_RELEASE_COMMON_MPI = $(foreach o, $(patsubst $(SOURCES_COMMON_DIR)/%.c, %.o, $(SOURCES_COMMON)), $(RELEASE_DIR_COMMON_MPI)/$o)
OBJ_RELEASE_SERIAL = $(foreach o, $(patsubst $(SOURCES_SERIAL_DIR)/%.c, %.o, $(SOURCES_SERIAL)), $(RELEASE_DIR_SERIAL)/$o)
OBJ_RELEASE_OPENMP = $(foreach o, $(patsubst $(SOURCES_OPENMP_DIR)/%.c, %.o, $(SOURCES_OPENMP)), $(RELEASE_DIR_OPENMP)/$o)
OBJ_RELEASE_MPI    = $(foreach o, $(patsubst $(SOURCES_MPI_DIR)/%.c, %.o, $(SOURCES_MPI)), $(RELEASE_DIR_MPI)/$o)

ifndef DFLAGS
	DFLAGS = -O0 -g -DDEBUG
endif
RFLAGS = -O3 -march=native -DNDEBUG -DNO_ASSERT -flto

override CFLAGS += -std=c11 -W -Wall -Wpedantic -pedantic -Werror=vla -flto
MPIFLAGS = `mpicc --showme:compile` `mpicc --showme:link`
PROG = recomender
OMPFLAGS = -fopenmp -Werror=unknown-pragmas
LFLAGS = -lm
CC = mpicc

all: debug _rename

debug: debug_serial debug_openmp debug_mpi

debug_serial: __debug_dir $(OBJ_DEBUG_COMMON) $(OBJ_DEBUG_SERIAL)
	@echo -e "\e[34mLinking $@\e[32m"
	$(CC) $(CFLAGS) -I$(HEADERS) $(OBJ_DEBUG_COMMON) $(OBJ_DEBUG_SERIAL) $(DFLAGS) -o $(DEBUG_DIR_SERIAL)/$(PROG) $(LFLAGS)
	@echo -en "\e[0m"

debug_openmp: __debug_dir $(OBJ_DEBUG_COMMON) $(OBJ_DEBUG_OPENMP)
	@echo -e "\e[34mLinking $@\e[32m"
	$(CC) $(CFLAGS) -I$(HEADERS) $(OBJ_DEBUG_COMMON) $(OBJ_DEBUG_OPENMP) $(DFLAGS) -o $(DEBUG_DIR_OPENMP)/$(PROG) $(OMPFLAGS) $(LFLAGS)
	@echo -en "\e[0m"

debug_mpi: __debug_dir $(OBJ_DEBUG_COMMON_MPI) $(OBJ_DEBUG_MPI)
	@echo -e "\e[34mLinking $@\e[32m"
	$(CC) $(CFLAGS) -I$(HEADERS) -rdynamic $(OBJ_DEBUG_COMMON_MPI) $(OBJ_DEBUG_MPI) $(DFLAGS) -o $(DEBUG_DIR_MPI)/$(PROG) -DMPI $(LFLAGS)
	@echo -en "\e[0m"

release: release_serial release_openmp release_mpi

release_serial: __release_dir $(OBJ_RELEASE_COMMON) $(OBJ_RELEASE_SERIAL)
	@echo -e "\e[34mLinking $@\e[32m"
	$(CC) $(CFLAGS) -I$(HEADERS) $(OBJ_RELEASE_COMMON) $(OBJ_RELEASE_SERIAL) $(RFLAGS) -o $(RELEASE_DIR_SERIAL)/$(PROG) $(LFLAGS)
	@echo -en "\e[0m"

release_openmp: __release_dir $(OBJ_RELEASE_COMMON) $(OBJ_RELEASE_OPENMP)
	@echo -e "\e[34mLinking $@\e[32m"
	$(CC) $(CFLAGS) -I$(HEADERS) $(OBJ_RELEASE_COMMON) $(OBJ_RELEASE_OPENMP) $(RFLAGS) -o $(RELEASE_DIR_OPENMP)/$(PROG) $(OMPFLAGS) $(LFLAGS)
	@echo -en "\e[0m"

release_mpi: __release_dir $(OBJ_RELEASE_COMMON_MPI) $(OBJ_RELEASE_MPI)
	@echo -e "\e[34mLinking $@\e[32m"
	$(CC) $(CFLAGS) -I$(HEADERS) $(OBJ_RELEASE_COMMON_MPI) $(OBJ_RELEASE_MPI) $(RFLAGS) -o $(RELEASE_DIR_MPI)/$(PROG) -DMPI $(LFLAGS)
	@echo -en "\e[0m"

$(DEBUG_DIR_COMMON)/%.o: $(SOURCES_COMMON)
	$(CC) $(patsubst %.o, %.c, $(patsubst $(DEBUG_DIR_COMMON)/%, $(SOURCES_COMMON_DIR)/%, $@)) $(CFLAGS) $(DFLAGS) -I$(HEADERS) -c -o $@

$(DEBUG_DIR_SERIAL)/%.o: $(SOURCES_SERIAL) $(SOURCES_COMMON)
	$(CC) $(patsubst %.o, %.c, $(patsubst $(DEBUG_DIR_SERIAL)/%, $(SOURCES_SERIAL_DIR)/%, $@)) $(CFLAGS) $(DFLAGS) -I$(HEADERS) -c -o $@

$(DEBUG_DIR_OPENMP)/%.o: $(SOURCES_OPENMP) $(SOURCES_COMMON)
	$(CC) $(patsubst %.o, %.c, $(patsubst $(DEBUG_DIR_OPENMP)/%, $(SOURCES_OPENMP_DIR)/%, $@)) $(CFLAGS) $(DFLAGS) -I$(HEADERS) -c -o $@ $(OMPFLAGS)

$(DEBUG_DIR_MPI)/%.o: $(SOURCES_MPI) $(SOURCES_COMMON)
	$(CC) $(patsubst %.o, %.c, $(patsubst $(DEBUG_DIR_MPI)/%, $(SOURCES_MPI_DIR)/%, $@)) $(CFLAGS) $(DFLAGS) -I$(HEADERS) -c -o $@ -DMPI

$(DEBUG_DIR_COMMON_MPI)/%.o: $(SOURCES_COMMON)
	$(CC) $(patsubst %.o, %.c, $(patsubst $(DEBUG_DIR_COMMON_MPI)/%, $(SOURCES_COMMON_DIR)/%, $@)) $(CFLAGS) $(DFLAGS) -I$(HEADERS) -c -o $@ -DMPI

$(RELEASE_DIR_COMMON)/%.o: $(SOURCES_COMMON)
	$(CC) $(patsubst %.o, %.c, $(patsubst $(RELEASE_DIR_COMMON)/%, $(SOURCES_COMMON_DIR)/%, $@)) $(CFLAGS) $(RFLAGS) -I$(HEADERS) -c -o $@

$(RELEASE_DIR_SERIAL)/%.o: $(SOURCES_SERIAL) $(SOURCES_COMMON)
	$(CC) $(patsubst %.o, %.c, $(patsubst $(RELEASE_DIR_SERIAL)/%, $(SOURCES_SERIAL_DIR)/%, $@)) $(CFLAGS) $(RFLAGS) -I$(HEADERS) -c -o $@

$(RELEASE_DIR_OPENMP)/%.o: $(SOURCES_OPENMP) $(SOURCES_COMMON)
	$(CC) $(patsubst %.o, %.c, $(patsubst $(RELEASE_DIR_OPENMP)/%, $(SOURCES_OPENMP_DIR)/%, $@)) $(CFLAGS) $(RFLAGS) -I$(HEADERS) -c -o $@ $(OMPFLAGS)

$(RELEASE_DIR_MPI)/%.o: $(SOURCES_MPI) $(SOURCES_COMMON)
	$(CC) $(patsubst %.o, %.c, $(patsubst $(RELEASE_DIR_MPI)/%, $(SOURCES_MPI_DIR)/%, $@)) $(CFLAGS) $(RFLAGS) -I$(HEADERS) -I$(HEADERS_MPI) -c -o $@ -DMPI

$(RELEASE_DIR_COMMON_MPI)/%.o: $(SOURCES_COMMON)
	$(CC) $(patsubst %.o, %.c, $(patsubst $(RELEASE_DIR_COMMON_MPI)/%, $(SOURCES_COMMON_DIR)/%, $@)) $(CFLAGS) $(DFLAGS) -I$(HEADERS) -c -o $@ -DMPI

test:
	./run_tests.sh

bench:
	./run_tests.sh bench

clean:
	rm -rf $(BUILD_DIR) matFact matFact-omp matFact-mpi

__debug_dir:
	@mkdir -p $(DEBUG_DIR_COMMON)
	@mkdir -p $(DEBUG_DIR_SERIAL)
	@mkdir -p $(DEBUG_DIR_OPENMP)
	@mkdir -p $(DEBUG_DIR_MPI)
	@mkdir -p $(DEBUG_DIR_COMMON_MPI)

__release_dir:
	@mkdir -p $(RELEASE_DIR_COMMON)
	@mkdir -p $(RELEASE_DIR_SERIAL)
	@mkdir -p $(RELEASE_DIR_OPENMP)
	@mkdir -p $(RELEASE_DIR_MPI)
	@mkdir -p $(RELEASE_DIR_COMMON_MPI)

_rename:
	cp $(DEBUG_DIR_SERIAL)/$(PROG) matFact
	cp $(DEBUG_DIR_OPENMP)/$(PROG) matFact-omp
	cp $(DEBUG_DIR_MPI)/$(PROG) matFact-mpi

print-% : ; @echo $* = $($*)

cpp-% : ; gcc -E $(OMPFLAGS) $*
