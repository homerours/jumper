#pragma once

#include <stdbool.h>

typedef enum CASE_MODE {
  CASE_MODE_sensitive,
  CASE_MODE_insensitive,
  CASE_MODE_semi_sensitive,
} CASE_MODE;

typedef enum SYNTAX {
  SYNTAX_extended,
  SYNTAX_fuzzy,
  SYNTAX_exact,
} SYNTAX;

typedef struct Query {
  char *query;
  bool *gap_allowed;
  int length;
} Query;

struct Query parse_query(const char *query, SYNTAX syntax);

void free_query(Query query);

int match_accuracy(const char *string, Query query, bool colors,
                   char **matched_string, CASE_MODE case_mode);
