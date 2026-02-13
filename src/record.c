#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "record.h"

static const double SHORT_DECAY = 2 * 1e-5;
static const double LONG_DECAY = 3 * 1e-7;

void parse_record(char *string, Record *rec) {
  // Warning: this modifies string
  char *current = string;
  rec->path = strtok_r(string, "|", &current);
  char *parsed = strtok_r(current, "|", &current);
  if (parsed == NULL || current == NULL || *parsed == '\0' ||
      *current == '\0') {
    fprintf(stderr, "ERROR: Invalid line format for the database file.\n"
                    "Lines have to be of the form "
                    "<path>|<number-of-visits>|<timestamp>.\n");
    exit(EXIT_FAILURE);
  }
  rec->n_visits = atof(parsed);
  rec->last_visit = atoll(current);
}

void update_record(Record *rec, long long now, double weight) {
  const double delta = now - rec->last_visit;
  rec->n_visits = weight + exp(-LONG_DECAY * delta) * rec->n_visits;
  rec->last_visit = now;
}

char *record_to_string(Record *rec) {
  const int n = strlen(rec->path) + 30;
  char *buffer = (char *)malloc(n * sizeof(char));
  if (!buffer)
    return NULL;
  if (snprintf(buffer, n, "%s|%f|%lld", rec->path, rec->n_visits,
               rec->last_visit) < 0) {
    free(buffer);
    return NULL;
  }
  return buffer;
}

double visits(double n_visits, double delta) {
  return exp(-LONG_DECAY * delta) * n_visits;
}

double frecency(double n_visits, double delta) {
  // log(0.1)  ~ -2.3, so we add 2.4 to ensure frecency is > 0
  return 2.4 + log(0.1 + 10 / (1 + delta * SHORT_DECAY) +
                   exp(-LONG_DECAY * delta) * n_visits);
}
