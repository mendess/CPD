#include <stdio.h>
#include <stdlib.h>
#include "matFact.h"
#include "parser.h"

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
    for(int i = 0; i < matrices.a_prime.n_rows; i++){
        printf("Row %d starts at position %ld\n", i, matrices.a_prime.row_pos[i]);
    }
}
