#include <ctype.h>
#include <errno.h>
#include <libgen.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#include "arguments.h"
#include "heap.h"
#include "matching.h"
#include "query.h"
#include "record.h"
#include "shell.h"
#include "textfile.h"

static inline bool exist(const char *path, TYPE type) {
  struct stat stats;
  if (stat(path, &stats) == 0) {
    return (((type == TYPE_directories) && (S_ISDIR(stats.st_mode) != 0)) ||
            ((type == TYPE_files) && (S_ISREG(stats.st_mode) != 0)));
  }
  return false;
}

static void clean_database(Arguments *args) {
  Textfile *f = file_open(args->file_path);
  if (!f){
	  return;
  }
  char *path_copy = strdup(args->file_path);
  char *dir = dirname(path_copy);
  char *tempname = (char *)malloc((strlen(dir) + 20) * sizeof(char));
  if (!tempname) {
    fprintf(stderr, "ERROR: failed to allocate %lu bytes.\n", strlen(dir) + 20);
    free(path_copy);
    exit(EXIT_FAILURE);
  }
  strcpy(tempname, dir);
  strcat(tempname, "/.jumper_XXXXXX");
  free(path_copy);
  const int temp_fd = mkstemp(tempname);
  if (temp_fd == -1) {
    fprintf(stderr, "ERROR: Could not create the temporary file %s\n",
            tempname);
    exit(EXIT_FAILURE);
  }
  FILE *temp = fdopen(temp_fd, "r+");
  if (!temp) {
    fprintf(stderr,
            "ERROR: Could not open the file descriptor %d of the temporary "
            "file %s\n",
            temp_fd, tempname);
    exit(EXIT_FAILURE);
  }

  // Preserve permissions from original file
  struct stat st;
  if (stat(args->file_path, &st) == 0) {
    fchmod(temp_fd, st.st_mode);
  }

  Record rec;
  int removed_count = 0;
  int kept_count = 0;

  while (next_line(f)) {
    parse_record(f->line, &rec);
    if (exist(rec.path, args->type)) {
      char *rec_string = record_to_string(&rec);
      if (fputs(rec_string, temp) == EOF || fputs("\n", temp) == EOF) {
        fprintf(stderr, "ERROR: Failed to write to temporary file\n");
        free(rec_string);
        fclose(temp);
        unlink(tempname);
        file_close(f);
        free(tempname);
        exit(EXIT_FAILURE);
      }
      free(rec_string);
      kept_count++;
    } else {
      removed_count++;
    }
  }
  file_close(f);
  fclose(temp);

  fprintf(stdout, "Cleaned %d non-existent %s (kept %d)\n",
          removed_count,
          args->type == TYPE_files ? "files" : "directories",
          kept_count);

  // Only rename if something was removed
  if (removed_count == 0) {
    unlink(tempname);
    free(tempname);
    return;
  }

  if (rename(tempname, args->file_path) != 0) {
    fprintf(stderr, "ERROR: Failed to replace database file: %s\n", strerror(errno));
    fprintf(stderr, "Cleaned data is in: %s\n", tempname);
    exit(EXIT_FAILURE);
  }
  free(tempname);
}

static void update_database(Arguments *args) {
  long long now = (long long)time(NULL);
  Textfile *f = file_open_rw(args->file_path);
  Record rec;
  const size_t n = strlen(args->key) + 2;
  char *prefix = (char *)malloc(n * sizeof(char));
  strcpy(prefix, args->key);
  prefix[n - 2] = '|';
  prefix[n - 1] = '\0';
  while (next_line(f)) {
    if (strncmp(f->line, prefix, n - 1) == 0) {
      // parse_record below will modify f->line
      // we have to make a copy of it
      char *buffer = strdup(f->line);
      parse_record(buffer, &rec);
      update_record(&rec, now, args->weight);
      char *rec_string = record_to_string(&rec);
      if (!rec_string) {
        file_close(f);
        fprintf(stderr, "ERROR: failed at formatting a record to string.\n");
        exit(EXIT_FAILURE);
      }
      overwrite_line(f, rec_string);
      free(rec_string);
      free(buffer);
      break;
    }
  }
  if (feof(f->fp)) {
    rec.n_visits = args->weight;
    rec.path = args->key;
    rec.last_visit = now;
    char *rec_string = record_to_string(&rec);
    if (!rec_string) {
      file_close(f);
      fprintf(stderr, "ERROR: failed at formatting a record to string.\n");
      exit(EXIT_FAILURE);
    }
    write_line(f, rec_string);
    free(rec_string);
  }
  file_close(f);
}

static void lookup(Arguments *args) {
  if (args->n_results <= 0) {
    return;
  }
  Textfile *f = file_open(args->file_path);
  if (!f){
	  return;
  }
  Heap *heap = heap_create(args->n_results);
  if (!heap) {
    fprintf(stderr, "ERROR: Could not allocate heap memory.");
    exit(EXIT_FAILURE);
  }
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
  char *matched_str;
  Record rec;
  while (next_line(f)) {
    parse_record(f->line, &rec);
    match_score = match_accuracy(rec.path, queries, args->highlight,
                                 &matched_str, args->case_mode);
    if (match_score > 0) {
      score = args->beta * 0.25 * match_score +
              frecency(rec.n_visits, now - rec.last_visit);
      if (heap_accept(heap, score) &&
          (!args->existing || exist(rec.path, args->type))) {
        if (heap_insert(heap, score, matched_str) != 0) {
          fprintf(stderr, "ERROR: Could not allocate heap memory.");
          exit(EXIT_FAILURE);
        }
      } else {
        free(matched_str);
      }
    }
  }
  heap_print(heap, args->print_scores, args->relative_to, args->home_tilde);
  file_close(f);
}

static int print_stats(const char *path) {
  int n_entries = 0;
  double total_visits = 0;

  long long now = (long long)time(NULL);

  Textfile *f = file_open(path);
  if (!f){
	  return -1;
  }
  Record rec;
  while (next_line(f)) {
    parse_record(f->line, &rec);
    n_entries++;
    total_visits += visits(rec.n_visits, now - rec.last_visit);
  }
  file_close(f);
  printf("%s:\n- %d entries\n- %.1f total_visits\n", path, n_entries,
         total_visits);
  return 0;
}

static void status_file(Arguments *args) {
  if (print_stats(args->file_path) != 0) {
    printf("File %s does not exist.\n", args->file_path);
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
  if (!args->file_path) {
    args->file_path = get_default_database_path(TYPE_directories);
    printf("DIRECTORIES: ");
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
    shell_setup(args->key, args->no_bind);
  }
  free(args);
  return EXIT_SUCCESS;
}
