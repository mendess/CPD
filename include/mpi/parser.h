#ifndef MPI_PARSER_H
#define MPI_PARSER_H
#include "common/matrix.h"
#include "common/parser.h"

ParserError parse_file_lt(char const* filename, Matrices* matrices);

ParserError parse_file_rt(char const* filename, VMatrices* matrices);

void recv_parsed_file(VMatrices* matrices);

#endif // MPI_PARSER_H
