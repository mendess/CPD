#include "common/parser.h"

#include "common/debug.h"
#include "mpi/cmatrix.h"
#include "mpi/parser.h"
#include "mpi/util.h"
#include "mpi/wrappers.h"

#include <assert.h>
#include <math.h>
#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static inline void transpose_items(Item* dest, Item const* src, size_t n) {
    Item const* const end = src + n;
    while (src != end) {
        dest->row = src->column;
        dest->column = src->row;
        dest->value = src->value;
        ++src;
        ++dest;
    }
}

static void broadcast_header(Header* const header, MPI_Request* const request) {
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

static ParserError spit_parse_a(
    StrIter* const iter,
    size_t const non_zero_elems,
    CompactMatrix* const a,
    CompactMatrix* const at) {

    size_t row, column;
    double value;
    size_t n_lines = 0;
    Item* item_buffer = a->items;
    Item* item_buffer_iter = item_buffer;
    Item const* const item_buffer_end = item_buffer + a->_total_items;
#ifndef NO_ASSERTS
    size_t out_of_bounds = 0;
#endif
    unsigned current_proc = 0;
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
        unsigned new_proc = proc_from_row_column(row, column, a->n_rows, a->n_cols);
        if (new_proc != current_proc) {
#ifndef NO_ASSERTS
            if (out_of_bounds > 0) {
                eprintln(
                    "Out of bounds, can only hold %zu items bro",
                    a->_total_items);
                eprintln("Tried to hold %zu items too many", out_of_bounds);
                debug_print_backtrace("Too Many Items");
            }
#endif
            size_t num_items = item_buffer_iter - item_buffer;
            if (current_proc == ROOT) {
                transpose_items(
                    at->items + a->current_items, item_buffer, num_items);
                a->current_items += num_items;
                item_buffer = a->items + a->current_items;
            } else {
                mpi_send_size(num_items, current_proc);
                mpi_send_items(item_buffer, num_items, current_proc);
                item_buffer_iter = item_buffer;
            }
            current_proc = new_proc;
        }
#ifndef NO_ASSERTS
        if (item_buffer_iter == item_buffer_end) {
            ++out_of_bounds;
            continue;
        }
#endif
        item_buffer_iter->row = row;
        item_buffer_iter->column = column;
        item_buffer_iter->value = value;
        ++item_buffer_iter;
    }
    size_t num_items = item_buffer_iter - item_buffer;
    if (current_proc == ROOT) {
        memcpy(
            a->items + a->current_items, item_buffer, num_items * sizeof(Item));
        transpose_items(at->items + a->current_items, item_buffer, num_items);
        a->current_items += num_items;
    } else {
        mpi_send_size(num_items, current_proc);
        mpi_send_items(item_buffer, num_items, current_proc);
    }
    for (unsigned proc = 1; proc < NPROCS; ++proc) {
        mpi_send_size(SIZE_MAX, proc);
    }
    at->current_items = at->_total_items = a->_total_items = a->current_items;
    if (a->current_items == 0) {
        free(a->items);
        free(at->items);
        a->items = at->items = NULL;
    } else {
        a->items = realloc(a->items, a->current_items * sizeof(Item));
        at->items = realloc(at->items, at->current_items * sizeof(Item));
    }

    if (n_lines < non_zero_elems) {
        fputs("Not as many elements as expected\n", stderr);
        return PARSER_ERROR_INVALID_FORMAT;
    }

    return PARSER_ERROR_OK;
}

/* static inline void print_header(Header const* const header) { */
/*     eprintln( */
/*         "Header {" */
/*         " users(i): %zu," */
/*         " items(j): %zu," */
/*         " features(k): %zu," */
/*         " non_zero_elems: %zu," */
/*         " alpha: %f," */
/*         " num_iterations: %zu" */
/*         " }", */
/*         header->users, */
/*         header->items, */
/*         header->features, */
/*         header->non_zero_elems, */
/*         header->alpha, */
/*         header->num_iterations); */
/* } */

ParserError parse_file_rt(char const* const filename, VMatrices* matrices) {
    char* contents = read_file(filename);
    if (contents == NULL)
        return PARSER_ERROR_IO;
    StrIter content_iter = {.str = contents};

    Header header = {0};
    ParserError error = parse_header(&content_iter, &header);
    if (error != PARSER_ERROR_OK) {
        free(contents);
        return error;
    }
    MPI_Request request;
    broadcast_header(&header, &request);

    CompactMatrix a;
    CompactMatrix at;

    if (should_work_alone(header.items, header.users)) {
        a = cmatrix_make(header.users, header.items, header.non_zero_elems);
        at = cmatrix_make(header.items, header.users, header.non_zero_elems);
        error = parse_matrix_a(&content_iter, header.non_zero_elems, &a, &at);
    } else {
        a = cmatrix_make_without_lengths(
            header.users, header.items, header.non_zero_elems);
        at = cmatrix_make_without_lengths(
            header.items, header.users, header.non_zero_elems);
        error = spit_parse_a(&content_iter, header.non_zero_elems, &a, &at);
    }
    free(contents);
    if (error != PARSER_ERROR_OK) {
        cmatrix_free(&a);
        cmatrix_free(&at);
        MPI_Wait(&request, NULL);
        return error;
    }
    cmatrix_sort(&at);
    ABounds abounds = a_bounds(0, a.n_rows, a.n_cols);
    *matrices = (VMatrices){
        .num_iterations = header.num_iterations,
        .alpha = header.alpha,
        .l = should_work_alone(header.items, header.users)
                 ? vmatrix_make(0, header.users, 0, header.features)
                 : vmatrix_make(
                       abounds.i.start, abounds.i.end, 0, header.features),
        .r = should_work_alone(header.items, header.users)
                 ? vmatrix_make(0, header.features, 0, header.items)
                 : vmatrix_make(
                       0, header.features, abounds.j.start, abounds.j.end),
        .a = a,
        .a_transpose = at,
    };
    MPI_Wait(&request, NULL);
    return PARSER_ERROR_OK;
}

bool recv_parsed_file(VMatrices* matrices) {
    Header header = {0};
    MPI_Request request;
    broadcast_header(&header, &request);
    MPI_Wait(&request, NULL);
    if (should_work_alone(header.items, header.users))
        return false;

    CompactMatrix a =
        cmatrix_make(header.users, header.items, header.non_zero_elems);
    CompactMatrix at =
        cmatrix_make(header.items, header.users, header.non_zero_elems);

    size_t read_so_far = 0;
    size_t line_size;
    while ((line_size = mpi_recv_size(ROOT)) != SIZE_MAX) {
        mpi_recv_items(a.items + read_so_far, line_size, ROOT);
        transpose_items(
            at.items + read_so_far, a.items + read_so_far, line_size);
        read_so_far += line_size;
    }
    if (read_so_far == 0) {
        free(a.items);
        free(at.items);
        a.items = at.items = NULL;
    } else {
        a.items = realloc(a.items, sizeof(Item) * read_so_far);
        at.items = realloc(at.items, sizeof(Item) * read_so_far);
    }
    at.current_items = at._total_items = a.current_items = a._total_items =
        read_so_far;
    cmatrix_sort(&at);
    ABounds abounds = a_bounds(ME, a.n_rows, a.n_cols);
    *matrices = (VMatrices){
        .num_iterations = header.num_iterations,
        .alpha = header.alpha,
        .l = vmatrix_make(abounds.i.start, abounds.i.end, 0, header.features),
        .r = vmatrix_make(0, header.features, abounds.j.start, abounds.j.end),
        .a = a,
        .a_transpose = at,
    };
    return true;
}
