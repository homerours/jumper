#include <stdbool.h>

enum MODE {
  MODE_search,
  MODE_add,
};

typedef struct Arguments {
  const char *file_path;
  const char *key;
  enum MODE mode;
  int n_results;
  bool highlight;
  bool print_scores;
  double frecency_multiplier;
} Arguments;

Arguments *parse_arguments(int argc, char **argv);
