#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "arguments.h"
#include "heap.h"
#include "matching.h"
#include "query.h"
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

static void update_database(Arguments *args) {
  FILE *fp = fopen(args->file_path, "r+");
  if (fp == NULL) {
    fprintf(stderr, "ERROR: File %s not found\n", args->file_path);
    exit(EXIT_FAILURE);
  }

  Record rec;
  long long now = (long long)time(NULL);
  char *line = NULL;
  size_t len;
  long int position = ftell(fp);

  while (getline(&line, &len, fp) != -1) {
    parse_record(line, &rec);
    if (strcmp(rec.path, args->key) == 0) {
      update_record(&rec, now, args->weight);
      char *rec_string = record_to_string(&rec);
      int record_length = strlen(rec_string);
      long int new_position = ftell(fp);

      if (record_length <= new_position - position - 1) {
        // New record has the same length or is shorter than the current one
        // one can simply overwrite the corresponding chars
        fseek(fp, position, SEEK_SET);
        fwrite(rec_string, sizeof(char), record_length, fp);
        // and add some padding if needed
        while (record_length < new_position - position - 1) {
          fwrite(" ", sizeof(char), 1, fp);
          record_length++;
        }
      } else {
        // New record is longer than the current one
        // We have to copy the rest of the file
        fseek(fp, new_position - 1, SEEK_SET);
        size_t file_tail_size;
        char *file_tail = file_to_buffer(fp, &file_tail_size);
        fseek(fp, position, SEEK_SET);
        fwrite(rec_string, sizeof(char), record_length, fp);
        fwrite(file_tail, sizeof(char), file_tail_size, fp);
        free(file_tail);
      }
      free(rec_string);
      break;
    }
    position = ftell(fp);
  }
  if (feof(fp)) {
    rec.n_visits = args->weight;
    rec.path = args->key;
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

static void lookup(Arguments *args) {
  FILE *fp = fopen(args->file_path, "r");
  if (fp == NULL) {
    fprintf(stderr, "ERROR: File %s not found\n", args->file_path);
    exit(EXIT_FAILURE);
  }

  Heap *heap = new_heap(args->n_results);
  Record rec;
  Queries queries;
  if (args->syntax == SYNTAX_extended) {
    queries = make_extended_queries(args->key, args->orderless);
  } else {
    Query query = make_normal_query(args->key, args->syntax == SYNTAX_fuzzy);
    queries.queries = &query;
    queries.n = 1;
  }

  // for (int i = 0; i < queries.n; i++) {
  //   printf("%d. %s (%f)\n", i, queries.queries[i].query,
  //          queries.queries[i].alignment);
  // }
  long long now = (long long)time(NULL);
  double match_score;
  double score;
  char *line = NULL, *matched_str;
  size_t len;
  while (getline(&line, &len, fp) != -1) {
    parse_record(line, &rec);
    match_score = match_accuracy(rec.path, queries, args->highlight,
                                 &matched_str, args->case_mode);
    if (match_score > 0) {
      score = args->beta * 0.25 * match_score +
              frecency(rec.n_visits, now - rec.last_visit);
      insert(heap, score, matched_str);
    }
  }
  print_heap(heap, args->print_scores, args->relative_to, args->home_tilde);
  fclose(fp);
  free_queries(queries);
  if (line) {
    free(line);
  }
}

int main(int argc, char **argv) {
  Arguments *args = parse_arguments(argc, argv);
  if (args->mode == MODE_search) {
    lookup(args);
  } else {
    update_database(args);
  }
  free(args);
  return EXIT_SUCCESS;
}
