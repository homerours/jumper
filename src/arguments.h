#include <stdbool.h>

enum MODE {
  MODE_search,
  MODE_add,
};

typedef struct Arguments {
  const char *file_path;
  const char *key;
  double beta;
  double weight;
  bool highlight;
  bool print_scores;
  int n_results;
  enum MODE mode;
} Arguments;

Arguments *parse_arguments(int argc, char **argv);
