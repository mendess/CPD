#include "matFact.h"
#include "parser.h"

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

    puts("After first iteration \n");
    Matrix b = iter(&matrices);
    matrix_print(&b);
    matrices_free(&matrices);
    return EXIT_SUCCESS;
}
