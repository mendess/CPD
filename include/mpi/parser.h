#ifndef MPI_PARSER_H
#define MPI_PARSER_H
#include "common/matrix.h"
#include "common/parser.h"

ParserError parse_file_lt(char const* filename, Matrices* matrices);

#endif // MPI_PARSER_H
