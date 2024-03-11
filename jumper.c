#include <ctype.h>
#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "arguments.h"
#include "heap.h"
#include "matching.h"
#include "record.h"

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

static double frecency(double rank, double time) {
  double days = time / (3600.0 * 24.0);
  double multiplier = 1.0;
  if (days < 0.04) { // < 1 hour
    multiplier = 4.0;
  } else if (days < 1) {
    multiplier = 2.0;
  } else if (days < 10) {
    multiplier = 1.2;
  }
  return multiplier * log(1.0 + rank);
}

static void update_database(const char *file, const char *key) {
  FILE *fp = fopen(file, "r+");
  if (fp == NULL) {
    fprintf(stderr, "ERROR: File %s not found\n", file);
    exit(EXIT_FAILURE);
  }

  Record rec;
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
      const int record_length = strlen(rec_string);
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
    const int record_length = strlen(rec_string);
    fwrite(rec_string, sizeof(char), record_length, fp);
    fwrite("\n", sizeof(char), 1, fp);
    free(rec_string);
  }
  fclose(fp);
  if (line) {
    free(line);
  }
}

static void lookup(const char *file, const char *key, int n, double fm,
                   bool colors, bool print_scores) {
  FILE *fp = fopen(file, "r");
  if (fp == NULL) {
    printf("ERROR: File %s not found\n", file);
    exit(EXIT_FAILURE);
  }

  Heap *heap = new_heap(n);
  Record rec;

  int match_score;
  double now = (double)time(NULL), score;
  char *line = NULL, *matched_str;
  size_t len;
  while (getline(&line, &len, fp) != -1) {
    parse_record(line, &rec);
    matched_str = match(rec.path, key, colors, &match_score);
    if (match_score > 0) {
      score = match_score + fm * frecency(rec.n_visits, now - rec.last_visit);
      insert(heap, score, matched_str);
    }
  }
  print_heap(heap, print_scores);
  fclose(fp);
  if (line) {
    free(line);
  }
}

int main(int argc, char **argv) {
  Arguments *args = parse_arguments(argc, argv);
  if (args->mode == MODE_search) {
    lookup(args->file_path, args->key, args->n_results,
           args->frecency_multiplier, args->highlight, args->print_scores);
  } else {
    update_database(args->file_path, args->key);
  }
  free(args);
  return 0;
}
