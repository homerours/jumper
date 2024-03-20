#pragma once

typedef struct Record {
  const char *path;
  double n_visits;
  long long last_visit;
} Record;

void parse_record(char *string, Record *rec);

void update_record(Record *rec, long long now);

char *record_to_string(Record *rec);

double frecency(double n_visits, double delta);
