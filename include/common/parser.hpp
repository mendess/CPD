#ifndef PARSER_H
#define PARSER_H
#include "matrix.hpp"
#include "result.hpp"

#include <stddef.h>
#include <string>

namespace parser {
enum class ParserError {
    IO,
    INVALID_FORMAT,
};

auto parse(char const* filename)
    -> result::Result<matrix::Matrices, ParserError>;
} // namespace parser

#endif // PARSER_H
