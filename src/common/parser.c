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

typedef enum FormatSpec {
    DOUBLE,
    SIZE_T,
} FormatSpec;

static size_t scan_line(
    StrIter* const s_iter, size_t n_specs, FormatSpec const* format, ...) {
    va_list args;
    va_start(args, format);
    size_t formats_read = 0;
    FormatSpec const* const end = format + n_specs;
    for (; format != end; ++format) {
        switch (*format) {
            case DOUBLE: {
                double* const arg = va_arg(args, double*);
                char* end = NULL;
                *arg = strtod(s_iter->str, &end);
                if (*arg == 0.0 && end == s_iter->str) goto END;
                s_iter->str = end;
                break;
            }
            case SIZE_T: {
                size_t* const arg = va_arg(args, size_t*);
                char* end = NULL;
                *arg = strtoull(s_iter->str, &end, 10);
                if (*arg == 0 && end == s_iter->str) goto END;
                s_iter->str = end;
                break;
            }
            default:
                abort();
        }
        ++formats_read;
    }
END:
    va_end(args);
    return formats_read;
}

ParserError parse_header(StrIter* const iter, Header* const p) {
    size_t num_iterations;
    if (scan_line(iter, 1, &(FormatSpec){SIZE_T}, &num_iterations) != 1) {
        fputs("Failed to get number of iterations\n", stderr);
        return PARSER_ERROR_INVALID_FORMAT;
    }
    double alpha;
    if (scan_line(iter, 1, &(FormatSpec){DOUBLE}, &alpha) != 1) {
        fputs("Failed to get alpha\n", stderr);
        return PARSER_ERROR_INVALID_FORMAT;
    }
    size_t features;
    if (scan_line(iter, 1, &(FormatSpec){SIZE_T}, &features) != 1) {
        fputs("Failed to get number of features\n", stderr);
        return PARSER_ERROR_INVALID_FORMAT;
    }
    size_t users, items, non_zero_elems;
    if (scan_line(
            iter,
            3,
            (FormatSpec[]){SIZE_T, SIZE_T, SIZE_T},
            &users,
            &items,
            &non_zero_elems) != 3) {
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
    CompactMatrix* const a,
    CompactMatrix* const a2) {
    size_t row, column;
    double value;
    size_t n_lines = 0;
    while (scan_line(
               iter,
               3,
               (FormatSpec[]){SIZE_T, SIZE_T, DOUBLE},
               &row,
               &column,
               &value) == 3) {
        ++n_lines;
        if (row >= a->n_rows || column >= a->n_cols) {
            fprintf(
                stderr,
                "Invalid row or column values at line %zu."
                " Max (%zu, %zu), got (%zu, %zu)\n",
                n_lines,
                a->n_rows,
                a->n_cols,
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
        cmatrix_add(a, row, column, value);
        cmatrix_add(a2, column, row, value);
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

    CompactMatrix a =
        cmatrix_make(header.users, header.items, header.non_zero_elems);
    a.row_pos[0] = 0;
    a.row_pos[a.n_rows] = header.non_zero_elems;
    CompactMatrix a_transpose =
        cmatrix_make(header.items, header.users, header.non_zero_elems);
    //write(2,"test\n", 6);
    a_transpose.row_pos[0] = 0;
    a_transpose.row_pos[a_transpose.n_rows] = header.non_zero_elems;

    error = parse_matrix_a(
        &content_iter, header.non_zero_elems, &a, &a_transpose);
    free(contents);
    if (error != PARSER_ERROR_OK) {
        cmatrix_free(&a);
        cmatrix_free(&a_transpose);
        return error;
    }
    // Transposed
    Matrix l = matrix_make(header.users, header.features);
    Matrix r = matrix_make(header.features, header.items);
    cmatrix_sort(&a_transpose);
    // TODO: passar para o iter
    random_fill_LR(header.features, &l, &r);
    *matrices = (Matrices){
        .num_iterations = header.num_iterations,
        .alpha = header.alpha,
        .l = l,
        .r = r,
        .a = a,
        .a_transpose = a_transpose,
    };
    return PARSER_ERROR_OK;
}
