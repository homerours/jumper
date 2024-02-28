#pragma once

extern const int MAX_HEAP_SIZE;

typedef struct Heap Heap;

Heap *new_heap(int max_elements);

void insert(Heap *heap, double priority, char *path);

void free_heap(Heap *heap);

void print_heap(Heap *heap, bool print_scores);
