#include "common/parser.h"
#include "mpi/parser.h"

#include <stdlib.h>

ParserError parse_file_lt(char const* filename, Matrices* matrices) {
    char* contents = read_file(filename);
    if (contents == NULL) return PARSER_ERROR_IO;
    StrIter content_iter = {.str = contents};

    Header header;
    ParserError error = parse_header(&content_iter, &header);
    if (error != PARSER_ERROR_OK) {
        free(contents);
        return error;
    }

    CompactMatrix a =
        cmatrix_make(header.users, header.items, header.non_zero_elems);
    CompactMatrix a_transpose =
        cmatrix_make(header.items, header.users, header.non_zero_elems);

    error = parse_matrix_a(
        &content_iter, header.non_zero_elems, &a, &a_transpose);
    free(contents);
    if (error != PARSER_ERROR_OK) {
        cmatrix_free(&a);
        cmatrix_free(&a_transpose);
        return error;
    }
    // Transposed
    Matrix lt = matrix_make(header.features, header.items);
    Matrix r = matrix_make(header.features, header.items);
    cmatrix_sort(&a_transpose);
    *matrices = (Matrices){
        .num_iterations = header.num_iterations,
        .alpha = header.alpha,
        .l = lt,
        .r = r,
        .a = a,
        .a_transpose = a_transpose,
    };
    return PARSER_ERROR_OK;
}
