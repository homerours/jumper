#pragma once

typedef struct item item;

typedef struct heap heap;

heap *new_heap(int max_elements);

void add(heap *heap, double priority, char *path);

void free_heap(heap *heap);

void print_sorted(heap *heap);
