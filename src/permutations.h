#pragma once

typedef struct Permutation {
  int n;
  int i;
  int *values;
  int *c;
  int alignment;
} Permutation;

Permutation *init_permutation(int n);
bool next_permutation(Permutation *p);
void free_permutation(Permutation *p);
