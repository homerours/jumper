#pragma once

#include <stdbool.h>

enum CASE_MODE {
  CASE_MODE_sensitive,
  CASE_MODE_insensitive,
  CASE_MODE_semi_sensitive,
};

int match_accuracy(const char *string, const char *query, bool colors,
                   char **matched_string, enum CASE_MODE case_mode);
