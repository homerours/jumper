#pragma once

typedef struct Permutation {
  int n;
  int i; // internal index
  int *values;
  int *c; // internal array
  int alignment;
  // alignement is n (n-1) / 2 - inversions
} Permutation;

Permutation *init_permutation(int n);
bool next_permutation(Permutation *p);
void free_permutation(Permutation *p);
