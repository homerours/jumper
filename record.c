#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "record.h"

static const double SHORT_DECAY = 1e-4;
static const double LONG_DECAY = 3 * 1e-7;

void parse_record(char *string, Record *rec) {
  char *current = string;
  rec->path = strtok_r(string, "|", &current);
  rec->n_visits = atof(strtok_r(current, "|", &current));
  rec->last_visit = atoi(current);
}

void update_record(Record *rec, int now) {
  int delta = now - rec->last_visit;
  rec->n_visits = 1 + exp(-LONG_DECAY * delta) * rec->n_visits;
  rec->last_visit = now;
}

char *record_to_string(Record *rec) {
  int n = strlen(rec->path) + 30;
  char *buffer = (char *)malloc(n * sizeof(char));
  snprintf(buffer, n, "%s|%f|%d", rec->path, rec->n_visits, rec->last_visit);
  return buffer;
}

double frecency(double n_visits, double delta) {
  return log(1.0 + 10 * exp(-SHORT_DECAY * delta) +
             exp(-LONG_DECAY * delta) * n_visits);
}
