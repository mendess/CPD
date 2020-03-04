#include "vector.h"

#include <stdlib.h>

Vec vec_make(size_t capacity) {
    return (Vec){
        .size = 0,
        .capacity = 0,
        .data = malloc(sizeof(int) * capacity),
    };
}

bool vec_push(Vec* v, int value) {
    if (v->size != v->capacity) {
        v->data[v->size++] = value;
        return true;
    }
    return false;
}

int vec_at(Vec const* v, size_t index) { return v->data[index]; }
