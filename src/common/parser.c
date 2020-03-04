#include "parser.h"

#include <fcntl.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#define RAND01 ((double) random() / (double) RAND_MAX)

static Matrix matrix_make(size_t rows, size_t columns);

static char* read_file(char const* const filename) {
    int const fd = open(filename, O_RDONLY);
    if (fd == -1) {
        perror("recomender: failed to open input file");
        return NULL;
    }
    struct stat metadata;
    if (fstat(fd, &metadata) != 0) {
        perror("recomender: failed getting input file metadata");
        return NULL;
    }
    char* file = malloc(metadata.st_size + 1);
    ssize_t const amount_read = read(fd, file, metadata.st_size);
    if (amount_read == -1) {
        perror("recomender: failed to read input file");
        free(file);
        return NULL;
    } else if (amount_read < metadata.st_size) {
        perror("recomender: failed to read entire file");
        free(file);
        return NULL;
    }
    file[metadata.st_size] = '\0';
    return file;
}

static void random_fill_LR(size_t nF, Matrix* l, Matrix* r) {
    srandom(0);

    for (MatrixIter i = matrix_iter_full(l); i.iter != i.end; ++i.iter) {
        *i.iter = RAND01 / (double) nF; // Why is this division being done?
    }

    for (MatrixIter i = matrix_iter_full(r); i.iter != i.end; ++i.iter) {
        *i.iter = RAND01 / (double) nF;
    }
}

typedef struct StrIter {
    char const* str;
} StrIter;

static int scan_line(StrIter* s_iter, char const* const format, ...) {
    va_list args;
    va_start(args, format);
    int formats_read = vsscanf(s_iter->str, format, args);
    va_end(args);
    while (*s_iter->str != '\n') {
        if (*s_iter->str == '\0') return formats_read;
        ++s_iter->str;
    }
    return formats_read;
}

typedef struct Params {
    size_t features;
    size_t users;
    size_t items;
    size_t non_zero_elems;
    float alpha;
    unsigned int num_iterations;
} Params;

ParserError parse_params(StrIter* iter, Params* p) {
    unsigned int num_iterations = 0;
    if (scan_line(iter, "%zu\n", &num_iterations) != 1) {
        fputs("Failed to get number of iterations", stderr);
        return PARSER_ERROR_INVALID_FORMAT;
    }
    float alpha;
    if (scan_line(iter, "%f", &alpha) != 1) {
        fputs("Failed to get alpha", stderr);
        return PARSER_ERROR_INVALID_FORMAT;
    }
    unsigned int features;
    if (scan_line(iter, "%i", &features) != 1) {
        fputs("Failed to get number of features", stderr);
        return PARSER_ERROR_INVALID_FORMAT;
    }
    unsigned int users, items, non_zero_elems;
    if (scan_line(iter, "%i %i %i", &users, &items, &non_zero_elems) != 3) {
        fputs("Failed to get matrix A information", stderr);
        return PARSER_ERROR_INVALID_FORMAT;
    }
    *p = (Params){
        .num_iterations = num_iterations,
        .alpha = alpha,
        .features = features,
        .users = users,
        .items = items,
        .non_zero_elems = non_zero_elems,
    };
    return PARSER_ERROR_OK;
}

ParserError parse_matrix_a(StrIter* iter, size_t non_zero_elems, Matrix* a) {
    size_t row, column;
    double value;
    size_t n_lines = 0;
    while (scan_line(iter, "%zu %zu %lf", &row, &column, &value) != 3) {
        // Perdi o nice scan direto para a matriz que tinhas.
        // Mas acedo menos vezes a matriz que ta longe e passo mais tempo
        // em variaveis "locais" :thinking:
        ++n_lines;
        fprintf(stderr, "line %zu: %zu %zu %lf", n_lines, row, column, value);
        if (row >= a->rows || column >= a->columns) {
            fprintf(
                stderr,
                "Invalid row or column values at line %zu."
                " Max (%zu, %zu), got (%zu, %zu)\n",
                n_lines,
                a->rows,
                a->columns,
                row,
                column);
            return PARSER_ERROR_INVALID_FORMAT;
        } else if (n_lines >= non_zero_elems) {
            fputs("More elements than expected", stderr);
            return PARSER_ERROR_INVALID_FORMAT;
        } else if (0.0 > value || value < 5.0) {
            fprintf(
                stderr,
                "Invalid matrix value at line %zu: %lf\n",
                n_lines,
                value);
        }
        *matrix_at_mut(a, row, column) = value;
    }

    if (n_lines < non_zero_elems) {
        fputs("Not as many elements as expected", stderr);
        return PARSER_ERROR_INVALID_FORMAT;
    }

    return PARSER_ERROR_OK;
}

ParserError parse_file(char const* const filename, Matrixes* matrixes) {
    char* contents = read_file(filename);
    if (contents == NULL) return PARSER_ERROR_IO;
    StrIter content_iter = {.str = contents};

    Params params;
    if (parse_params(&content_iter, &params) != PARSER_ERROR_OK) {
        free(contents);
        return PARSER_ERROR_INVALID_FORMAT;
    }

    Matrix a = matrix_make(params.users, params.items);
    if (PARSER_ERROR_OK !=
        parse_matrix_a(&content_iter, params.non_zero_elems, &a)) {
        free(contents);
    }
    Matrix l = matrix_make(params.users, params.features);
    Matrix r = matrix_make(params.features, params.items);
    random_fill_LR(params.features, &l, &r);
    *matrixes = (Matrixes){
        .num_iterations = params.num_iterations,
        .alpha = params.alpha,
        .a = a,
        .l = l,
        .r = r,
    };
    return PARSER_ERROR_OK;
}

Matrix matrix_make(size_t rows, size_t columns) {
    return (Matrix){.rows = rows,
                    .columns = columns,
                    .data = malloc(rows * columns * sizeof(double))};
}

double const* matrix_at(Matrix const* m, size_t row, size_t column) {
    return m->data + (row * m->columns + column);
}

double* matrix_at_mut(Matrix* m, size_t row, size_t column) {
    return m->data + (row * m->columns + column);
}

MatrixIter matrix_iter_row(Matrix* m, size_t row) {
    return (MatrixIter){
        .iter = m->data + (row * m->columns),
        .end = m->data + (row * m->columns + m->columns),
    };
}

MatrixIter matrix_iter_full(Matrix* m) {
    return (MatrixIter){
        .iter = m->data + (0 * m->columns),
        .end = m->data + ((m->rows - 1) * m->columns + m->columns),
    };
}

void matrix_print(Matrix const* m) {
    for (size_t i = 0; i < m->rows; i++) {
        for (size_t j = 0; j < m->columns; j++) {
            printf("%.3lf   ", *matrix_at(m, i, j));
        }
        printf("\n");
    }
    printf("\n");
}
