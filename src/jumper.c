#include <ctype.h>
#include <libgen.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>

#include "arguments.h"
#include "heap.h"
#include "matching.h"
#include "query.h"
#include "record.h"
#include "shell.h"

static char *file_to_buffer(FILE *fp) {
  long int position = ftell(fp);
  fseek(fp, 0, SEEK_END);
  long int end_position = ftell(fp);
  fseek(fp, position, SEEK_SET);
  size_t size = end_position - position;
  char *buffer = (char *)malloc((size + 1) * sizeof(char));
  size_t n_read = fread(buffer, sizeof(char), size, fp);
  if (n_read != size) {
    fprintf(stderr, "ERROR: Could not read file.");
    exit(EXIT_FAILURE);
  }
  buffer[size] = '\0';
  return buffer;
}

static inline bool exist(const char *path, TYPE type) {
  struct stat stats;
  if (stat(path, &stats) == 0) {
    return (((type == TYPE_directories) && (S_ISDIR(stats.st_mode) != 0)) ||
            ((type == TYPE_files) && (S_ISREG(stats.st_mode) != 0)));
  }
  return false;
}

static void clean_database(Arguments *args) {
  FILE *fp = fopen(args->file_path, "r+");
  if (fp == NULL) {
    fprintf(stderr, "ERROR: File %s not found\n", args->file_path);
    exit(EXIT_FAILURE);
  }
  char *dir = dirname(strdup(args->file_path));
  char *tempname = (char *)malloc((strlen(dir) + 20) * sizeof(char));
  strcpy(tempname, dir);
  strcat(tempname, "/.jumper_XXXXXX");
  const int temp_fd = mkstemp(tempname);
  if (temp_fd == -1) {
    fprintf(stderr, "ERROR: Could not create the temporary file %s\n",
            tempname);
    exit(EXIT_FAILURE);
  }
  FILE *temp = fdopen(temp_fd, "r+");
  if (temp == NULL) {
    fprintf(stderr,
            "ERROR: Could not open the file descriptor %d of the temporary "
            "file %s\n",
            temp_fd, tempname);
    exit(EXIT_FAILURE);
  }

  Record rec;
  char *line = NULL;
  size_t len;

  while (getline(&line, &len, fp) != -1) {
    parse_record(line, &rec);
    if (exist(rec.path, args->type)) {
      char *rec_string = record_to_string(&rec);
      fputs(rec_string, temp);
      fputs("\n", temp);
      free(rec_string);
    }
  }
  fclose(fp);
  fclose(temp);
  rename(tempname, args->file_path);
  free(tempname);
  if (line) {
    free(line);
  }
}

static void update_database(Arguments *args) {
  FILE *fp = fopen(args->file_path, "r+");
  if (fp == NULL) {
    // Database does not exist, we create it.
    fp = fopen(args->file_path, "w+");
    if (fp == NULL) {
      fprintf(stderr, "ERROR: Couldn't open file %s.\n", args->file_path);
      exit(EXIT_FAILURE);
    }
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
        fputs(rec_string, fp);
        // and add some padding if needed
        while (record_length < new_position - position - 1) {
          fputs(" ", fp);
          record_length++;
        }
      } else {
        // New record is longer than the current one
        // We have to copy the rest of the file
        fseek(fp, new_position - 1, SEEK_SET);
        char *file_tail = file_to_buffer(fp);
        fseek(fp, position, SEEK_SET);
        fputs(rec_string, fp);
        fputs(file_tail, fp);
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
    fputs(rec_string, fp);
    fputs("\n", fp);
    free(rec_string);
  }
  fclose(fp);
  if (line) {
    free(line);
  }
}

static void lookup(Arguments *args) {
  if (args->n_results <= 0) {
    return;
  }
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
    Query query = make_standard_query(args->key, args->syntax == SYNTAX_fuzzy);
    queries.queries = &query;
    queries.n = 1;
  }

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

static int print_stats(const char *path) {
  FILE *fp = fopen(path, "r+");
  if (fp == NULL) {
    return 1;
  }
  int n_entries = 0;
  double total_visits = 0;

  Record rec;
  long long now = (long long)time(NULL);
  char *line = NULL;
  size_t len;

  while (getline(&line, &len, fp) != -1) {
    n_entries++;
    parse_record(line, &rec);
    total_visits += visits(rec.n_visits, now - rec.last_visit);
  }
  fclose(fp);
  if (line) {
    free(line);
  }
  printf("%s:\n- %d entries\n- %.1f total_visits\n", path, n_entries,
         total_visits);
  return 0;
}

static void status_file(Arguments *args) {
  if (print_stats(args->file_path) != 0) {
    printf("ERROR, couldn't open %s\n", args->file_path);
  } else if (args->n_results > 0) {
    printf("Top %d entries", args->n_results);
    if (strlen(args->key) > 0) {
      printf(" matching %s:\n", args->key);
    } else {
      printf(":\n");
    }
    lookup(args);
  }
}
static void status(Arguments *args) {
  if (args->file_path == NULL) {
    args->file_path = get_default_database_path(TYPE_directories);
    printf("\nDIRECTORIES: ");
    status_file(args);
    args->file_path = get_default_database_path(TYPE_files);
    printf("\nFILES: ");
    status_file(args);
  } else {
    status_file(args);
  }
}

int main(int argc, char **argv) {
  Arguments *args = parse_arguments(argc, argv);
  if (args->mode == MODE_search) {
    lookup(args);
  } else if (args->mode == MODE_update) {
    update_database(args);
  } else if (args->mode == MODE_status) {
    status(args);
  } else if (args->mode == MODE_clean) {
    clean_database(args);
  } else if (args->mode == MODE_shell) {
    shell_setup(args->key);
  }
  free(args);
  return EXIT_SUCCESS;
}
