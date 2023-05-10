#include "array.h"
#include <assert.h>
#include <stdlib.h>

array *new_arr(int cap) {
  array *ar = (array *)calloc(1, sizeof(array));
  ar->length = 0;
  ar->capacity = cap;
  ar->elem = (void **)calloc(cap, sizeof(void *));
  return ar;
}

void *arr_get(int idx, array *arr) {
  if (idx < arr->length)
    return arr->elem[idx];
  return NULL;
}

static void arr_resize(array *arr) {
  arr->elem = realloc(arr->elem, (arr->capacity << 1) * sizeof(void *));
  assert(arr->elem);
  arr->capacity <<= 1;
}

void arr_push(array *arr, void *e) {
  if (arr->length >= arr->capacity) {
    arr_resize(arr);
  }
  arr->elem[arr->length++] = e;
}