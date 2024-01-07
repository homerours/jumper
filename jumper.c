#include <ctype.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "heap.h"

static inline int max(int x, int y) { return ((x) > (y) ? x : y); }

static double match(char *string, char *query, int key_length) {
  if (*query == '\0') {
    return 1.0;
  }
  char *t = string;
  int total_length = 0, max_length = 0;
  char *q = query, *qinit = query;
  while (*t != 0 && *q != 0) {
    if (*q == ' ') {
      total_length += q - qinit + 1;
      qinit = ++q;
      max_length = 0;
    } else if (tolower(*t) == tolower(*q)) {
      q++;
      t++;
    } else if (q != qinit) {
      max_length = max(q - qinit, max_length);
      q = qinit;
    } else {
      t++;
    }
  }
  max_length = max(q - qinit, max_length);
  total_length += max_length;
  double s = ((double)total_length) / ((double)key_length);
  if (s < 0.3) {
    return 0.0;
  }
  if (s == 1.0) {
    if (strchr(t, '/') == NULL) {
      return 10000.0;
    }
    return 100.0;
  }
  return s;
}

typedef struct record {
  char *path;
  int nVisits;
  int timeLastVisit;
} record;

static void parse_record(char *string, record *rec) {
  char *current = string;
  rec->path = strtok_r(string, "|", &current);
  rec->nVisits = atoi(strtok_r(current, "|", &current));
  rec->timeLastVisit = atoi(strtok_r(current, "|", &current));
}

static double frecent(double rank, double time) {
  return 1.0 + log(1.0 + rank) / (0.0001 * time + 1.0);
}

static void lookup(char *file, char *key, int n) {
  FILE *fp = fopen(file, "r");
  if (fp == NULL) {
    printf("ERROR: File %s not found\n", file);
    exit(EXIT_FAILURE);
  }

  heap *heap = new_heap(n);
  record rec;

  int queueLength = 0;
  int key_length = strlen(key);

  double s, value, delta;

  double now = (double)time(NULL);

  char *line = NULL;
  size_t len = 0;
  while (getline(&line, &len, fp) != -1) {
    parse_record(line, &rec);
    s = match(rec.path, key, key_length);
    if (s > 0) {
      delta = now - rec.timeLastVisit;
      value = s * frecent(rec.nVisits, delta);
      add(heap, value, strdup(rec.path));
    }
  }
  print_sorted(heap);
  free_heap(heap);
  fclose(fp);
  if (line) {
    free(line);
  }
}

int main(int argc, char **argv) {
  char *file;
  int c, n = 20;
  while ((c = getopt(argc, argv, "hf:n:")) != -1)
    switch (c) {
    case 'f':
      file = optarg;
      break;
    case 'n':
      n = atoi(optarg);
      if (n <= 0) {
        fprintf(stderr,
                "ERROR: The number of results -n has to be positive!\n");
      }
      break;
    case 'h':
      printf(
          "Usage: %s -f <logfile-of-visited-folders> -n <number-of-entries>\n",
          argv[0]);
      break;
    case '?':
      if (optopt == 'n')
        fprintf(stderr, "Option -%c requires an argument.\n", optopt);
      else if (isprint(optopt))
        fprintf(stderr, "Unknown option `-%c'.\n", optopt);
      else
        fprintf(stderr, "Unknown option character `\\x%x'.\n", optopt);
      return 1;
    default:
      abort();
    }

  if (optind == argc) {
    return 0;
  }
  if (optind < argc - 1) {
    fprintf(stderr, "ERROR: Got more than 1 query: ");
    for (int i = optind; i < argc; i++) {
      fprintf(stderr, "%s ", argv[i]);
    }
    return 1;
  }

  char *key = argv[argc - 1];
  lookup(file, key, n);
  return 0;
}
