#include "parser.h"

#include <stdio.h>
#include <stdlib.h>

int main(int argc, char const** argv) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s [filename]\n", argv[0]);
        return EXIT_FAILURE;
    }
    Matrixes matrixes;
    ParserError error = parse_file(argv[1], &matrixes);
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

    matrix_print(&matrixes.a);
    matrix_print(&matrixes.l);
    matrix_print(&matrixes.r);

    return EXIT_SUCCESS;
}
