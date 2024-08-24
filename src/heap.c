#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct Item {
  double value;
  char *path;
} Item;

static void new_item(Item *item, double value, char *path) {
  item->value = value;
  item->path = path;
}

static inline void swap(Item *item1, Item *item2) {
  Item tmp = *item1;
  *item1 = *item2;
  *item2 = tmp;
}

typedef struct Heap {
  int n_items;    // number of items currently stored
  int size;       // maximum number of items
  int alloc_size; // current allocated size
  Item *items;
} Heap;

Heap *heap_create(int size) {
  Heap *heap = (Heap *)malloc(sizeof(Heap));
  if (!heap) {
    return NULL;
  }
  heap->n_items = 0;
  heap->size = size;
  heap->alloc_size = 512;
  heap->items = (Item *)malloc(size * sizeof(Item));
  if (!heap->items) {
    return NULL;
  } else {
    return heap;
  }
}

void heap_free(Heap *heap) { free(heap->items); }

static int heap_grow(Heap *heap) {
  int newsize = 2 * heap->alloc_size;
  Item *newitems = realloc(heap->items, newsize * sizeof(Item));
  if (!newitems) {
    heap_free(heap);
    return -1;
  }
  heap->items = newitems;
  heap->alloc_size = newsize;
  return 0;
}

static inline int parent(int i) { return ((i + 1) >> 1) - 1; }
static inline int child(int i) { return (i << 1) + 1; }

static void bubble_down(Heap *heap, int elem) {
  int n = elem;
  int nchild = child(n);
  Item *items = heap->items;
  while (nchild < heap->n_items) {
    Item *parent = items + n;
    if (heap->n_items > nchild + 1 &&
        items[nchild].value > items[nchild + 1].value)
      nchild++;
    Item *childe = items + nchild;
    if (parent->value <= childe->value)
      break;
    swap(parent, childe);
    n = nchild;
    nchild = child(n);
  }
}

static void heapify(Heap *heap) {
  for (int i = parent(heap->n_items - 1); i != -1; i--) {
    bubble_down(heap, i);
  }
}

int heap_insert(Heap *heap, double value, char *path) {
  if (heap->n_items == heap->alloc_size && heap->size > heap->alloc_size) {
    if (heap_grow(heap) != 0)
      return -1;
  }
  if (heap->n_items == heap->size) {
    if (value > heap->items->value) {
      free(heap->items->path);
      new_item(heap->items, value, path);
      bubble_down(heap, 0);
    } else {
      free(path);
    }
  } else {
    new_item(heap->items + heap->n_items, value, path);
    heap->n_items++;
    if (heap->n_items == heap->size) {
      heapify(heap);
    }
  }
  return 0;
}

void heap_print(Heap *heap, bool print_scores, const char *relative_to,
                bool tilde) {
  const int n = heap->n_items;
  if (n != heap->size) {
    heapify(heap);
  }
  while (heap->n_items > 1) {
    heap->n_items--;
    swap(heap->items, heap->items + heap->n_items);
    bubble_down(heap, 0);
  }
  const char *home_folder;
  int home_len = 0;
  if (tilde) {
    home_folder = getenv("HOME");
    home_len = strlen(home_folder);
  }
  const int relative_len = (relative_to == NULL) ? 0 : strlen(relative_to);
  const char *path;
  for (int i = 0; i < n; i++) {
    if (print_scores) {
      printf("%.3f  ", heap->items[i].value);
    }
    path = heap->items[i].path;
    if (relative_len > 0 && strncmp(path, relative_to, relative_len) == 0 &&
        path[relative_len] == '/') {
      printf("%s\n", path + relative_len + 1);
    } else if (tilde && strncmp(path, home_folder, home_len) == 0 &&
               path[home_len] == '/') {
      printf("~%s\n", path + home_len);
    } else {
      printf("%s\n", path);
    }
    free(heap->items[i].path);
  }
  heap_free(heap);
}
