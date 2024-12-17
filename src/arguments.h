#pragma once

#include "matching.h"
#include <stdbool.h>

typedef enum MODE {
  MODE_search,
  MODE_clean,
  MODE_update,
  MODE_shell,
  MODE_status,
} MODE;

typedef enum TYPE {
  TYPE_directories,
  TYPE_files,
  TYPE_undefined,
} TYPE;

typedef struct Arguments {
  const char *file_path;
  const char *key;
  double beta;
  double weight;
  bool highlight;
  bool print_scores;
  bool home_tilde;
  bool orderless;
  bool existing;
  bool no_bind;
  TYPE type;
  int n_results;
  const char *relative_to;
  MODE mode;
  SYNTAX syntax;
  CASE_MODE case_mode;
} Arguments;

Arguments *parse_arguments(int argc, char **argv);
char *get_default_database_path(TYPE type);
