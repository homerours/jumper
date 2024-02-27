#include <stdio.h>
#include <stdlib.h>

const int MAX_HEAP_SIZE = 10000;

typedef struct item {
  double value;
  char *path;
} item;

static void new_item(item *item, double value, char *path) {
  item->value = value;
  item->path = path;
  // printf("new item: %s (%f)\n", path,value);
}

static inline void swap(item *item1, item *item2) {
  item tmp = *item1;
  *item1 = *item2;
  *item2 = tmp;
}

typedef struct heap {
  int n_items;
  int size;
  item *items;
} heap;


heap *new_heap(int size) {
  heap *heap = malloc(sizeof(struct heap));
  heap->n_items = 0;
  heap->size = size > MAX_HEAP_SIZE ? MAX_HEAP_SIZE : size;
  heap->items = (item *)malloc(size * sizeof(item));
  return heap;
}

static inline int parent(int i) { return ((i + 1) >> 1) - 1; }
static inline int child(int i) { return (i << 1) + 1; }

static void bubble_down(heap *heap, int elem) {
  int n = elem;
  int nchild = child(n);
  item *items = heap->items;
  while (nchild < heap->n_items) {
    item *parent = items + n;
    if (heap->n_items > nchild + 1 &&
        items[nchild].value > items[nchild + 1].value)
      nchild++;
    item *childe = items + nchild;
    if (parent->value <= childe->value)
      break;
    swap(parent, childe);
    n = nchild;
    nchild = child(n);
  }
}

static void heapify(heap *heap) {
  for (int i = parent(heap->n_items - 1); i != -1; i--) {
    bubble_down(heap, i);
  }
}

void insert(heap *heap, double value, char *path) {
  if (heap->n_items == heap->size) {
    if (value > heap->items->value) {
      new_item(heap->items, value, path);
      bubble_down(heap, 0);
    }
  } else {
    new_item(heap->items + heap->n_items, value, path);
    heap->n_items++;
    if (heap->n_items == heap->size) {
      heapify(heap);
    }
  }
}

void free_heap(heap *heap) {
  free(heap->items);
  free(heap);
}

void print_sorted(heap *heap) {
  int n = heap->n_items;
  if (n != heap->size) {
    heapify(heap);
  }
  while (heap->n_items > 1) {
    heap->n_items--;
    swap(heap->items, heap->items + heap->n_items);
    bubble_down(heap, 0);
  }
  for (int i = 0; i < n; i++) {
    printf("%s\n", heap->items[i].path);
  }
}
