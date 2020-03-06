#ifndef PARSER_H
#define PARSER_H
#include "matrix.h"

#include <stddef.h>

typedef struct Matrixes {
    size_t num_iterations;
    double alpha;
    Matrix a;
    Matrix l;
    Matrix r;
} Matrixes;

typedef enum ParserError {
    PARSER_ERROR_OK,
    PARSER_ERROR_IO,
    PARSER_ERROR_INVALID_FORMAT,
} ParserError;

ParserError parse_file(char const* filename, Matrixes* matrixes);

#endif // PARSER_H
