#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct Record {
  const char *path;
  int n_visits;
  int last_visit;
} Record;

void parse_record(char *string, Record *rec) {
  char *current = string;
  rec->path = strtok_r(string, "|", &current);
  rec->n_visits = atoi(strtok_r(current, "|", &current));
  rec->last_visit = atoi(current);
}

char *record_to_string(Record *rec) {
  int n = strlen(rec->path) + 30;
  char *buffer = (char *)malloc(n * sizeof(char));
  snprintf(buffer, n, "%s|%d|%d", rec->path, rec->n_visits, rec->last_visit);
  return buffer;
}
