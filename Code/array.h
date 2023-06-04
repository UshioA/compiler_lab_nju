#ifndef __CMM_ARRAY_H__
#define __CMM_ARRAY_H__
#include <stdlib.h>
typedef struct array {
  int capacity;
  int length;
  void **elem;
} array;

array *new_arr(int cap);
void* arr_get(int idx, array* arr);
void arr_set(int idx, void* val, array* arr);
void arr_push(array* arr, void* e);
#endif