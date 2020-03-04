#include "vector.h"
#include <stdio.h>

int main(void) {
    Vec v = vec_make(3);
    vec_push(&v, 42);
    vec_push(&v, 314);
    vec_push(&v, 0);
    printf("%d\n", vec_at(&v, 2));
}
