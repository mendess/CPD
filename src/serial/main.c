#include "serial/matFact.h"
#include "common/parser.h"

#include <stdio.h>
#include <stdlib.h>

int main(int argc, char const** argv) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s [filename]\n", argv[0]);
        return EXIT_FAILURE;
    }
    Matrices matrices;
    ParserError error = parse_file(argv[1], &matrices);
    switch (error) {
        case PARSER_ERROR_IO:
            fputs("IO Error\n", stderr);
            return EXIT_FAILURE;
        case PARSER_ERROR_INVALID_FORMAT:
            fputs("Format Error\n", stderr);
            return EXIT_FAILURE;
        default:
            break;
    }

    random_fill_LR(&matrices.l, &matrices.r);
    Matrix b = iter(&matrices);
    print_output(&matrices, &b);
    matrices_free(&matrices);
    matrix_free(&b);
    return EXIT_SUCCESS;
}
