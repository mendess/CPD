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
    // matrix_print(&b);
    double max;
    size_t max_pos;
    for (size_t row = 0; row < matrices.a.rows; row++) {
        max = 0;
        max_pos = 0;
        for (size_t column = 0; column < matrices.a.columns; column++) {
            if (*matrix_at(&matrices.a, row, column) == 0) {
                double aux = *matrix_at(&b, row, column);
                if (aux > max) {
                    max = aux;
                    max_pos = column;
                }
            }
        }
        printf("%ld\n", max_pos);
    }
    matrices_free(&matrices);
    return EXIT_SUCCESS;
}
