#ifndef PARSER_H
#define PARSER_H
#include "common/matrix.h"

#include <stddef.h>

typedef struct StrIter {
    char const* str;
} StrIter;

typedef struct Header {
    size_t features;
    size_t users;
    size_t items;
    size_t non_zero_elems;
    double alpha;
    unsigned int num_iterations;
} Header;

typedef enum ParserError {
    PARSER_ERROR_OK,
    PARSER_ERROR_IO,
    PARSER_ERROR_INVALID_FORMAT,
} ParserError;

char* read_file(char const* filename);

ParserError parse_header(StrIter* iter, Header* p);

ParserError parse_matrix_a(
    StrIter* iter, size_t non_zero_elems, CompactMatrix* a, CompactMatrix* a2);

ParserError parse_file(char const* filename, Matrices* matrices);

#endif // PARSER_H
