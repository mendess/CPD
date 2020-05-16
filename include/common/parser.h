#ifndef PARSER_H
#define PARSER_H
#include "common/matrix.h"

#include <stddef.h>

typedef struct {
    char const* str;
} StrIter;

typedef struct {
    size_t features;
    size_t users;
    size_t items;
    size_t non_zero_elems;
    double alpha;
    size_t num_iterations;
} Header;

typedef enum {
    PARSER_ERROR_OK,
    PARSER_ERROR_IO,
    PARSER_ERROR_INVALID_FORMAT,
} ParserError;

typedef enum {
    DOUBLE,
    SIZE_T,
} FormatSpec;

char* read_file(char const* filename);

size_t scan_line(
    StrIter* const s_iter, size_t n_specs, FormatSpec const* format, ...);

ParserError parse_header(StrIter* iter, Header* p);

ParserError parse_matrix_a(
    StrIter* iter, size_t non_zero_elems, CompactMatrix* a, CompactMatrix* a2);

ParserError parse_file(char const* filename, Matrices* matrices);

#endif // PARSER_H
