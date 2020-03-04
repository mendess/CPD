#ifndef VECTOR_H
#define VECTOR_H
#include <stddef.h>
#include <stdbool.h>

typedef struct Vec {
    size_t size;
    size_t capacity;
    int* data;
} Vec;


Vec vec_make(size_t capacity);

bool vec_push(Vec* v, int value);

int vec_at(Vec const*v, size_t index);

#endif // VECTOR_H
