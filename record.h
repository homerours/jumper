#pragma once

typedef struct Record {
  const char *path;
  int n_visits;
  int last_visit;
} Record;

void parse_record(char *string, Record *rec);

char *record_to_string(Record *rec);
