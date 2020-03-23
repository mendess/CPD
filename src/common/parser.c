#include "parser.h"

#include "compact_matrix.h"

#include <fcntl.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

typedef struct StrIter {
    char const* str;
} StrIter;

typedef struct Header {
    size_t features;
    size_t users;
    size_t items;
    size_t non_zero_elems;
    double alpha;
    unsigned int num_iterations;
} Header;

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

static int scan_line(StrIter* const s_iter, char const* const format, ...) {
    va_list args;
    va_start(args, format);
    int const formats_read = vsscanf(s_iter->str, format, args);
    va_end(args);
    while (*s_iter->str != '\n') {
        if (*s_iter->str == '\0') return formats_read;
        ++s_iter->str;
    }
    ++s_iter->str;
    return formats_read;
}

ParserError parse_header(StrIter* const iter, Header* const p) {
    size_t num_iterations = 0;
    if (scan_line(iter, "%zu\n", &num_iterations) != 1) {
        fputs("Failed to get number of iterations\n", stderr);
        return PARSER_ERROR_INVALID_FORMAT;
    }
    double alpha;
    if (scan_line(iter, "%lf", &alpha) != 1) {
        fputs("Failed to get alpha\n", stderr);
        return PARSER_ERROR_INVALID_FORMAT;
    }
    size_t features;
    if (scan_line(iter, "%zu", &features) != 1) {
        fputs("Failed to get number of features\n", stderr);
        return PARSER_ERROR_INVALID_FORMAT;
    }
    size_t users, items, non_zero_elems;
    if (scan_line(iter, "%zu %zu %zu", &users, &items, &non_zero_elems) != 3) {
        fputs("Failed to get matrix A information\n", stderr);
        return PARSER_ERROR_INVALID_FORMAT;
    }
    *p = (Header){
        .num_iterations = num_iterations,
        .alpha = alpha,
        .features = features,
        .users = users,
        .items = items,
        .non_zero_elems = non_zero_elems,
    };
    return PARSER_ERROR_OK;
}

ParserError parse_matrix_a(
    StrIter* const iter,
    size_t const non_zero_elems,
    CompactMatrix* const a_prime,
    CompactMatrix* const a_prime2) {
    size_t row, column;
    double value;
    size_t n_lines = 0;
    while (scan_line(iter, "%zu %zu %lf", &row, &column, &value) == 3) {
        ++n_lines;
        if (row >= a_prime->n_rows || column >= a_prime->n_cols) {
            fprintf(
                stderr,
                "Invalid row or column values at line %zu."
                " Max (%zu, %zu), got (%zu, %zu)\n",
                n_lines,
                a_prime->n_rows,
                a_prime->n_cols,
                row,
                column);
            return PARSER_ERROR_INVALID_FORMAT;
        } else if (n_lines > non_zero_elems) {
            fputs("More elements than expected\n", stderr);
            return PARSER_ERROR_INVALID_FORMAT;
        } else if (0.0 > value || value > 5.0) {
            fprintf(
                stderr,
                "Invalid matrix value at line %zu: %lf\n",
                n_lines,
                value);
            return PARSER_ERROR_INVALID_FORMAT;
        }
        cmatrix_add(a_prime, row, column, value);
        cmatrix_add(a_prime2, column, row, value);
    }

    if (n_lines < non_zero_elems) {
        fputs("Not as many elements as expected\n", stderr);
        return PARSER_ERROR_INVALID_FORMAT;
    }

    return PARSER_ERROR_OK;
}

ParserError parse_file(char const* const filename, Matrices* const matrices) {
    char* contents = read_file(filename);
    if (contents == NULL) return PARSER_ERROR_IO;
    StrIter content_iter = {.str = contents};

    Header header;
    ParserError error = parse_header(&content_iter, &header);
    if (error != PARSER_ERROR_OK) {
        free(contents);
        return error;
    }

    CompactMatrix a_prime =
        cmatrix_make(header.users, header.items, header.non_zero_elems);
    CompactMatrix a_prime_transpose =
        cmatrix_make(header.items, header.users, header.non_zero_elems);
    error = parse_matrix_a(
        &content_iter, header.non_zero_elems, &a_prime, &a_prime_transpose);
    free(contents);
    if (error != PARSER_ERROR_OK) {
        cmatrix_free(&a_prime);
        cmatrix_free(&a_prime_transpose);
        return error;
    }
    Matrix l = matrix_make(header.users, header.features);
    Matrix r = matrix_make(header.features, header.items);
    cmatrix_sort(&a_prime_transpose);
    random_fill_LR(header.features, &l, &r);
    *matrices = (Matrices){
        .num_iterations = header.num_iterations,
        .alpha = header.alpha,
        .l = l,
        .r = r,
        .a_prime = a_prime,
        .a_prime_transpose = a_prime_transpose,
    };
    return PARSER_ERROR_OK;
}
