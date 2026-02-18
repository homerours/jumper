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
#include "glob.h"
#include "heap.h"
#include "matching.h"
#include "progress_bar.h"
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
  if (!f) {
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

  // Count total lines for progress tracking
  int total_lines = 0;
  while (next_line(f)) {
    total_lines++;
  }
  file_close(f);

  // Reopen file for processing
  f = file_open(args->file_path);
  if (!f) {
    fclose(temp);
    unlink(tempname);
    free(tempname);
    return;
  }

  char **filters = read_filters(args->filters);
  Record rec;
  int removed_count = 0;
  int kept_count = 0;
  int current_line = 0;

  char *type_name = args->type == TYPE_files ? "files" : "directories";
  fprintf(stdout, "Cleaning %s' database...\n", type_name);
  while (next_line(f)) {
    parse_record(f->line, &rec);
    if (!glob_match_list(filters, rec.path) && exist(rec.path, args->type)) {
      char *rec_string = record_to_string(&rec);
      if (fputs(rec_string, temp) == EOF || fputs("\n", temp) == EOF) {
        fprintf(stderr, "\nERROR: Failed to write to temporary file\n");
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
    current_line++;
    progress_bar(current_line, total_lines);
  }
  file_close(f);
  fclose(temp);
  free_filters(filters);

  fprintf(stdout, "Cleaned %d %s (kept %d)\n", removed_count, type_name,
          kept_count);

  // Only rename if something was removed
  if (removed_count == 0) {
    unlink(tempname);
    free(tempname);
    return;
  }

  if (args->dry_run) {
    fprintf(stdout, "Dry run: filtered data saved to %s\n", tempname);
    fprintf(stdout, "Original database unchanged: %s\n", args->file_path);
    free(tempname);
    return;
  }

  if (rename(tempname, args->file_path) != 0) {
    fprintf(stderr, "ERROR: Failed to replace database file: %s\n",
            strerror(errno));
    fprintf(stderr, "Cleaned data is in: %s\n", tempname);
    exit(EXIT_FAILURE);
  }
  free(tempname);
}

static void clean_both_databases(Arguments *args) {
  // Clean files database
  args->type = TYPE_files;
  args->file_path = get_default_database_path(TYPE_files);
  clean_database(args);

  printf("\n");

  // Clean directories database
  args->type = TYPE_directories;
  args->file_path = get_default_database_path(TYPE_directories);
  clean_database(args);
}

static void update_database(Arguments *args) {
  char **filters = read_filters(args->filters);
  if (glob_match_list(filters, args->key)) {
    free_filters(filters);
    return;
  }
  free_filters(filters);

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
  free(prefix);
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

static void lookup(Arguments *args, const char *prefix) {
  if (args->n_results <= 0) {
    return;
  }
  Textfile *f = file_open(args->file_path);
  if (!f) {
    return;
  }
  char **filters = read_filters(args->filters);
  Heap *heap = heap_create(args->n_results);
  if (!heap) {
    fprintf(stderr, "ERROR: Could not allocate heap memory.\n");
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
    if (glob_match_list(filters, rec.path)) {
      continue;
    }
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
  heap_print(heap, args->print_scores, args->relative_to, args->home_tilde,
             prefix);
  file_close(f);
  free_filters(filters);
}

static int count_filters(const char *path) {
  char **filters = read_filters(path);
  if (!filters)
    return 0;
  int count = 0;
  while (filters[count])
    count++;
  free_filters(filters);
  return count;
}

static void print_config(const Arguments *args, bool color) {
  if (color)
    printf("\033[1m");
  printf("CONFIG:");
  if (color)
    printf("\033[0m");
  printf("\n");

  char *dir_path = get_default_database_path(TYPE_directories);
  char *files_path = get_default_database_path(TYPE_files);

  printf("  directories: %s%s\n", dir_path,
         getenv("__JUMPER_FOLDERS") ? " (via $__JUMPER_FOLDERS)" : "");
  printf("  files: %s%s\n", files_path,
         getenv("__JUMPER_FILES") ? " (via $__JUMPER_FILES)" : "");

  free(dir_path);
  free(files_path);

  if (args->filters) {
    int n = count_filters(args->filters);
    printf("  filters: %s (%d pattern%s)%s\n", args->filters, n,
           n == 1 ? "" : "s",
           getenv("__JUMPER_FILTERS") ? " (via $__JUMPER_FILTERS)" : "");
  } else {
    printf("  filters: (disabled)\n");
  }
}

static int get_stats(const char *path, int *n_entries, double *total_visits) {
  *n_entries = 0;
  *total_visits = 0;

  long long now = (long long)time(NULL);

  Textfile *f = file_open(path);
  if (!f) {
    return -1;
  }
  Record rec;
  while (next_line(f)) {
    parse_record(f->line, &rec);
    (*n_entries)++;
    *total_visits += visits(rec.n_visits, now - rec.last_visit);
  }
  file_close(f);
  return 0;
}

static void status_file(const char *label, Arguments *args, bool color) {
  const char *header = label ? label : args->file_path;

  if (color)
    printf("\033[1m");
  printf("%s:", header);
  if (color)
    printf("\033[0m");
  printf("\n");

  args->print_scores = true;

  int n_entries;
  double total_visits;
  if (get_stats(args->file_path, &n_entries, &total_visits) != 0) {
    printf("  File does not exist.\n");
    return;
  }

  printf("  %d entries, %.1f total visits\n", n_entries, total_visits);

  if (args->n_results > 0) {
    printf("  Top %d entries", args->n_results);
    if (strlen(args->key) > 0)
      printf(" matching %s", args->key);
    printf(":\n");
    lookup(args, "  ");
  }
}

static void status(Arguments *args) {
  bool color = isatty(STDOUT_FILENO);
  if (!args->file_path) {
    print_config(args, color);

    args->file_path = get_default_database_path(TYPE_directories);
    printf("\n");
    status_file("DIRECTORIES", args, color);

    args->file_path = get_default_database_path(TYPE_files);
    printf("\n");
    status_file("FILES", args, color);
  } else {
    status_file(NULL, args, color);
  }
}

int main(int argc, char **argv) {
  Arguments *args = parse_arguments(argc, argv);
  if (args->mode == MODE_search) {
    lookup(args, NULL);
  } else if (args->mode == MODE_update) {
    update_database(args);
  } else if (args->mode == MODE_status) {
    status(args);
  } else if (args->mode == MODE_clean) {
    if (args->type == TYPE_undefined) {
      clean_both_databases(args);
    } else {
      clean_database(args);
    }
  } else if (args->mode == MODE_shell) {
    shell_setup(args->key, args->no_bind);
  }
  free(args);
  return EXIT_SUCCESS;
}
