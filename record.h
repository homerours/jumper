#pragma once

typedef struct record {
  char *path;
  int n_visits;
  int last_visit;
} record;

void parse_record(char *string, record *rec);

char *record_to_string(record *rec);
