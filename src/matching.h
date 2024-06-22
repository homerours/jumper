#pragma once

#include <stdbool.h>

#include "query.h"

typedef enum CASE_MODE {
  CASE_MODE_sensitive,
  CASE_MODE_insensitive,
  CASE_MODE_semi_sensitive,
} CASE_MODE;



double match_accuracy(const char *string, Queries queries, bool colors, char **output,
                   CASE_MODE case_mode);
