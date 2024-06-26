#pragma once

#include <stdbool.h>
#include "matching.h"

typedef enum MODE {
  MODE_search,
  MODE_add,
} MODE;

typedef struct Arguments {
  const char *file_path;
  const char *key;
  double beta;
  double weight;
  bool highlight;
  bool print_scores;
  bool home_tilde;
  int n_results;
  const char *relative_to;
  MODE mode;
  SYNTAX syntax;
  CASE_MODE case_mode;
} Arguments;

Arguments *parse_arguments(int argc, char **argv);
