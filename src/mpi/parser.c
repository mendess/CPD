#include "common/parser.h"

#include "common/debug.h"
#include "mpi/cmatrix.h"
#include "mpi/parser.h"
#include "mpi/util.h"
#include "mpi/wrappers.h"

#include <assert.h>
#include <math.h>
#include <mpi.h>
#include <stdlib.h>
#include <string.h>

ParserError parse_file_lt(char const* const filename, Matrices* matrices) {
    char* contents = read_file(filename);
    if (contents == NULL)
        return PARSER_ERROR_IO;
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

    error =
        parse_matrix_a(&content_iter, header.non_zero_elems, &a, &a_transpose);
    free(contents);
    if (error != PARSER_ERROR_OK) {
        cmatrix_free(&a);
        cmatrix_free(&a_transpose);
        return error;
    }
    cmatrix_sort(&a_transpose);
    *matrices = (Matrices){
        .num_iterations = header.num_iterations,
        .alpha = header.alpha,
        .l = {.data = NULL, .rows = header.features, .columns = header.users},
        .r = {.data = NULL, .rows = header.features, .columns = header.items},
        .a = a,
        .a_transpose = a_transpose,
    };
    return PARSER_ERROR_OK;
}

/* typedef struct { */
/*     Slice i; */
/*     Slice j; */
/* } ABounds; */

/* static inline ABounds */
/* a_bounds(int const zone, size_t const rows, size_t const columns) { */
/*     int x = zone / CHECKER_BOARD_SIDE; */
/*     int y = zone % CHECKER_BOARD_SIDE; */
/*     Slice i_bounds = slice_rows(x, CHECKER_BOARD_SIDE, rows); */
/*     Slice j_bounds = slice_rows(y, CHECKER_BOARD_SIDE, columns); */
/*     return (ABounds){.i = i_bounds, .j = j_bounds}; */
/* } */

static inline int proc_from_row_column(
    size_t const row,
    size_t const column,
    size_t const rows,
    size_t const columns) {

    int x = proc_from_chunk(row, CHECKER_BOARD_SIDE, rows);
    int y = proc_from_chunk(column, CHECKER_BOARD_SIDE, columns);
    return x * CHECKER_BOARD_SIDE + y;
}

ParserError spit_parse_a(
    StrIter* const iter, size_t const non_zero_elems, CompactMatrix* const a) {

    size_t row, column;
    double value;
    size_t n_lines = 0;
    size_t item_buffer_len =
        a->n_rows + (CHECKER_BOARD_SIDE - 1) / CHECKER_BOARD_SIDE;
    Item* item_buffer = malloc(sizeof(Item) * item_buffer_len);
    Item* item_buffer_iter = item_buffer;
    int current_proc = 0;
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
            free(item_buffer);
            return PARSER_ERROR_INVALID_FORMAT;
        } else if (n_lines > non_zero_elems) {
            fputs("More elements than expected\n", stderr);
            free(item_buffer);
            return PARSER_ERROR_INVALID_FORMAT;
        } else if (0.0 > value || value > 5.0) {
            fprintf(
                stderr,
                "Invalid matrix value at line %zu: %lf\n",
                n_lines,
                value);
            free(item_buffer);
            return PARSER_ERROR_INVALID_FORMAT;
        }
        int new_proc = proc_from_row_column(row, column, a->n_rows, a->n_cols);
        if (new_proc != current_proc) {
            size_t num_items = item_buffer_iter - item_buffer;
            if (current_proc == ROOT) {
                memcpy(
                    a->items + a->current_items,
                    item_buffer,
                    num_items * sizeof(Item));
                a->current_items += num_items;
            } else {
                mpi_send_size(num_items, current_proc);
                mpi_send_items(item_buffer, num_items, current_proc);
            }
            item_buffer_iter = item_buffer;
            current_proc = new_proc;
        }
        item_buffer_iter->row = row;
        item_buffer_iter->column = column;
        item_buffer_iter->value = value;
        ++item_buffer_iter;
        //= (Item){.row = row, .column = column, .value = value};
    }
    int new_proc = proc_from_row_column(row, column, a->n_rows, a->n_cols);
    if (new_proc != current_proc) {
        size_t num_items = item_buffer_iter - item_buffer;
        if (current_proc == ROOT) {
            memcpy(
                a->items + a->current_items,
                item_buffer,
                num_items * sizeof(Item));
            a->current_items += num_items;
        } else {
            mpi_send_size(num_items, current_proc);
            mpi_send_items(item_buffer, num_items, current_proc);
        }
    }
    for (int proc = 1; proc < NPROCS; ++proc) {
        mpi_send_size(SIZE_MAX, proc);
    }

    free(item_buffer);

    if (n_lines < non_zero_elems) {
        fputs("Not as many elements as expected\n", stderr);
        return PARSER_ERROR_INVALID_FORMAT;
    }

    return PARSER_ERROR_OK;
}

static inline void
broadcast_header(Header* const header, MPI_Request* const request) {
    MPI_Datatype mpi_header_t;
    int blocklengths[] = {1, 1, 1, 1, 1, 1};
    MPI_Datatype types[] = {
        MPI_SIZE_T, MPI_SIZE_T, MPI_SIZE_T, MPI_SIZE_T, MPI_DOUBLE, MPI_SIZE_T};
    MPI_Aint offsets[] = {
        offsetof(Header, features),
        offsetof(Header, users),
        offsetof(Header, items),
        offsetof(Header, non_zero_elems),
        offsetof(Header, alpha),
        offsetof(Header, num_iterations)};
    enum { NUM_ITEMS = 6 };
    static_assert(
        sizeof(offsets) / sizeof(MPI_Aint) == NUM_ITEMS &&
            sizeof(blocklengths) / sizeof(int) == NUM_ITEMS &&
            sizeof(types) / sizeof(MPI_Datatype) == NUM_ITEMS,
        "Header MPI type member number inconsistent");
    MPI_Type_create_struct(
        NUM_ITEMS, blocklengths, offsets, types, &mpi_header_t);
    MPI_Type_commit(&mpi_header_t);
    MPI_Ibcast(header, 1, mpi_header_t, ROOT, MPI_COMM_WORLD, request);
}

ParserError parse_file_rt(char const* const filename, Matrices* matrices) {
    char* contents = read_file(filename);
    if (contents == NULL)
        return PARSER_ERROR_IO;
    StrIter content_iter = {.str = contents};

    Header header;
    ParserError error = parse_header(&content_iter, &header);
    if (error != PARSER_ERROR_OK) {
        free(contents);
        return error;
    }
    MPI_Request request;
    broadcast_header(&header, &request);

    CompactMatrix a =
        cmatrix_make(header.users, header.items, header.non_zero_elems);

    error = spit_parse_a(&content_iter, header.non_zero_elems, &a);
    free(contents);
    if (error != PARSER_ERROR_OK) {
        cmatrix_free(&a);
        MPI_Wait(&request, NULL);
        return error;
    }
    *matrices = (Matrices){
        .num_iterations = header.num_iterations,
        .alpha = header.alpha,
        .l = {.data = NULL, .rows = header.users, .columns = header.features},
        .r = {.data = NULL, .rows = header.items, .columns = header.features},
        .a = a,
        .a_transpose = {0},
    };
    MPI_Wait(&request, NULL);
    return PARSER_ERROR_OK;
}

void recv_parsed_file(Matrices* matrices) {
    Header header;
    MPI_Request request;
    broadcast_header(&header, &request);
    MPI_Wait(&request, NULL);

    CompactMatrix a =
        cmatrix_make(header.users, header.items, header.non_zero_elems);

    size_t read_so_far = 0;
    size_t line_size;
    while ((line_size = mpi_recv_size(ROOT)) != SIZE_MAX) {
        mpi_recv_items(a.items + read_so_far, line_size, ROOT);
        read_so_far += line_size;
    }
    if (read_so_far == 0) {
        free(a.items);
        a.items = NULL;
    } else {
        a.items = realloc(a.items, sizeof(Item) * read_so_far);
    }
    a.current_items = a._total_items = read_so_far;

    *matrices = (Matrices){
        .num_iterations = header.num_iterations,
        .alpha = header.alpha,
        .l = {.data = NULL, .rows = header.users, .columns = header.features},
        .r = {.data = NULL, .rows = header.items, .columns = header.features},
        .a = a,
        .a_transpose = {0},
    };
}
