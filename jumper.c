#include <ctype.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "heap.h"
#include "record.h"

static inline int max(int x, int y) { return ((x) > (y) ? x : y); }

const char *HELP_STRING = "Jumper: jump around your directories and files!\n\n\
- To find the closest matches to <query>:\n\
%s -f <logfile> -n <number-of-results> <query>\n\n\
- To update the database with <query>:\n\
%s -f <logfile> -a <query>\n";

static double match(char *string, char *query, int query_length) {
  if (*query == '\0') {
    return 1.0;
  }
  char *t = string, *q = query, *tb, *qb;
  int consecutive = 0, max_consecutive = 0;
  while (*t != 0 && *q != 0) {
    if (tolower(*t) == tolower(*q)) {
      if (consecutive == 0) {
        tb = t - 1;
        qb = q - 1;
        // Backward search
        while (qb >= query && tolower(*tb) == tolower(*qb)) {
          tb--;
          qb--;
          consecutive++;
        }
      }
      q++;
      consecutive++;
      max_consecutive = max(max_consecutive, consecutive);
    } else {
      if (consecutive == 1) {
        // Discard single char matches
        q--;
      }
      consecutive = 0;
    }
    t++;
  }
  if (consecutive == 1 && query_length > 1) {
    // Discard single char matches
    q--;
  }
  double s = ((double)(q - query)) / ((double)query_length);
  if (s < 0.3) {
    return 0.0;
  }
  if (s == 1.0) {
    if (strchr(t, '/') == NULL) {
      return max_consecutive * 100.0 + consecutive * 1000.0;
    }
    return max_consecutive * 100.0;
  }
  return max_consecutive * s;
}

static char *file_to_buffer(FILE *fp, size_t *size) {
  long int position = ftell(fp);
  fseek(fp, 0, SEEK_END);
  long int end_position = ftell(fp);
  fseek(fp, position, SEEK_SET);
  *size = end_position - position;
  char *buffer = (char *)malloc((*size + 1) * sizeof(char));
  size_t n_read = fread(buffer, sizeof(char), *size, fp);
  if (n_read != *size) {
    fprintf(stderr, "ERROR: Could not read file.");
    exit(EXIT_FAILURE);
  }
  buffer[*size] = '\0';
  return buffer;
}

static double frecentcy(double rank, double time) {
  return 1.0 + log(1.0 + rank) / (0.00005 * time + 1.0);
}

static void update_data(char *file, char *key) {
  FILE *fp = fopen(file, "r+");
  if (fp == NULL) {
    fprintf(stderr, "ERROR: File %s not found\n", file);
    exit(EXIT_FAILURE);
  }

  record rec;
  int now = (int)time(NULL);
  char *line = NULL;
  size_t len;
  long int position = ftell(fp);

  while (getline(&line, &len, fp) != -1) {
    parse_record(line, &rec);
    if (strcmp(rec.path, key) == 0) {
      rec.n_visits += 1;
      rec.last_visit = now;
      char *rec_string = record_to_string(&rec);
      int record_length = strlen(rec_string);
      long int new_position = ftell(fp);

      if (record_length > new_position - position - 1) {
        // New record is longer than the current one
        // We have to copy the rest of the file
        fseek(fp, new_position - 1, SEEK_SET);
        size_t file_tail_size;
        char *file_tail = file_to_buffer(fp, &file_tail_size);
        fseek(fp, position, SEEK_SET);
        fwrite(rec_string, sizeof(char), record_length, fp);
        fwrite(file_tail, sizeof(char), file_tail_size, fp);
        free(file_tail);
      } else {
        fseek(fp, position, SEEK_SET);
        fwrite(rec_string, sizeof(char), record_length, fp);
      }
      free(rec_string);
      break;
    }
    position = ftell(fp);
  }
  if (feof(fp)) {
    rec.n_visits = 1;
    rec.path = key;
    rec.last_visit = now;
    char *rec_string = record_to_string(&rec);
    int record_length = strlen(rec_string);
    fwrite(rec_string, sizeof(char), record_length, fp);
    fwrite("\n", sizeof(char), 1, fp);
    free(rec_string);
  }
  fclose(fp);
  if (line) {
    free(line);
  }
}

static void lookup(char *file, char *key, int n) {
  FILE *fp = fopen(file, "r");
  if (fp == NULL) {
    printf("ERROR: File %s not found\n", file);
    exit(EXIT_FAILURE);
  }

  heap *heap = new_heap(n);
  record rec;

  int key_length = strlen(key);
  double s, delta;
  double now = (double)time(NULL);

  char *line = NULL;
  size_t len;
  while (getline(&line, &len, fp) != -1) {
    parse_record(line, &rec);
    s = match(rec.path, key, key_length);
    if (s > 0) {
      delta = now - rec.last_visit;
      insert(heap, s * frecentcy(rec.n_visits, delta), strdup(rec.path));
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
  if (argc == 1) {
    printf(HELP_STRING, argv[0], argv[0]);
    return 0;
  }
  char *file = NULL;
  int c, n = MAX_HEAP_SIZE, mode = 0;
  while ((c = getopt(argc, argv, "hf:n:a")) != -1)
    switch (c) {
    case 'f':
      file = optarg;
      break;
    case 'a':
      mode = 1;
      break;
    case 'n':
      n = atoi(optarg);
      if (n <= 0) {
        fprintf(stderr,
                "ERROR: The number of results -n has to be positive!\n");
        return 1;
      }
      break;
    case 'h':
      printf(HELP_STRING, argv[0], argv[0]);
      break;
    case '?':
      if (optopt == 'f' || optopt == 'n') {
        return 1;
      }
      if (isprint(optopt))
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
    fprintf(stderr, "\n");
    return 1;
  }
  if (file == NULL) {
    fprintf(stderr, "ERROR: you must provide a data file with -f.\n");
    return 1;
  }
  char *key = argv[argc - 1];
  if (mode == 0) {
    lookup(file, key, n);
  } else if (*key != '\0') {
    update_data(file, key);
  }
  return 0;
}
