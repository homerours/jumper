#pragma once

typedef struct Heap Heap;

Heap *heap_create(int size);

int heap_insert(Heap *heap, double priority, char *path);

void heap_free(Heap *heap);

void heap_print(Heap *heap, bool print_scores, const char *relative_to,
                bool tilde);
