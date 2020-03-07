#include "parser.h"
#include "matFact.h"
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

    puts("Matrix A\n");
    matrix_print(&matrixes.a);
    puts("==========================================");
    puts("Matrix L\n");
    matrix_print(&matrixes.l);
    puts("==========================================");
    puts("Matrix R\n");
    matrix_print(&matrixes.r);

    matrix_b(&matrixes.l, &matrixes.r);
    next_iter(&matrixes.a, &matrixes.l, &matrixes.r, matrixes.alpha);

    puts("After first iteration \n");
    puts("Matrix A\n\n");
    matrix_print(&matrixes.a);
    puts("==========================================");
    puts("Matrix L\n\n");
    matrix_print(&matrixes.l);
    puts("==========================================");
    puts("Matrix R\n\n");
    matrix_print(&matrixes.r);

    return EXIT_SUCCESS;
}
