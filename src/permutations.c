#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "permutations.h"

// We generate all n! permutations using Heap's algorithm
Permutation *init_permutation(int n) {
  Permutation *p = (Permutation *)malloc(sizeof(Permutation));
  if (!p)
    return NULL;
  p->n = n;
  p->i = 1;
  p->alignment = (n * (n - 1)) / 2; // alignement is n (n-1) / 2 - inversions
  p->values = (int *)malloc(n * sizeof(int));
  if (!p->values)
    return NULL;
  p->c = (int *)malloc(n * sizeof(int));
  if (!p->c)
    return NULL;
  for (int i = 0; i < n; i++) {
    p->values[i] = i;
    p->c[i] = 0;
  }
  return p;
}

static inline void swap(Permutation *p, int i, int j) {
  int *v = p->values;
  p->alignment += 1 - 2 * (v[i] < v[j]);
  for (int k = i + 1; k < j; k++) {
    p->alignment += 2 - 2 * ((v[k] < v[j]) + (v[i] < v[k]));
  }
  int t = v[i];
  v[i] = v[j];
  v[j] = t;
}

bool next_permutation(Permutation *p) {
  while (p->i < p->n) {
    if (p->c[p->i] < p->i) {
      if (p->i % 2 == 0) {
        swap(p, 0, p->i);
      } else {
        swap(p, p->c[p->i], p->i);
      }
      p->c[p->i]++;
      p->i = 1;
      return true;
    } else {
      p->c[p->i] = 0;
      p->i++;
    }
  }
  return false;
}

void free_permutation(Permutation *p) {
  free(p->values);
  free(p->c);
  free(p);
}
