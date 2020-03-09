#ifndef PARSER_H
#define PARSER_H
#include "matrix.h"

#include <stddef.h>

typedef enum ParserError {
    PARSER_ERROR_OK,
    PARSER_ERROR_IO,
    PARSER_ERROR_INVALID_FORMAT,
} ParserError;

ParserError parse_file(char const* filename, Matrices* matrices);

#endif // PARSER_H
