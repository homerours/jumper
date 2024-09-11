#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "textfile.h"

Textfile *file_open(const char *path) {
  Textfile *f = (Textfile *)malloc(sizeof(Textfile));
  f->fp = fopen(path, "r");
  if (!f->fp) {
    fprintf(stderr, "ERROR: File %s not found\n", path);
    exit(EXIT_FAILURE);
  }
  f->line = NULL;
  return f;
}
Textfile *file_open_rw(const char *path) {
  Textfile *f = (Textfile *)malloc(sizeof(Textfile));
  f->fp = fopen(path, "r+");
  if (!f->fp) {
    // Textfile does not exist, we create it.
    f->fp = fopen(path, "w+");
    if (!f->fp) {
      fprintf(stderr, "ERROR: Couldn't open file %s.\n", path);
      exit(EXIT_FAILURE);
    }
  }
  f->line = NULL;
  return f;
}

static char *file_to_buffer(FILE *fp) {
  const long position = ftell(fp); // save current position
  fseek(fp, 0, SEEK_END);
  long end_position = ftell(fp);
  fseek(fp, position, SEEK_SET);
  const size_t size = end_position - position;
  char *buffer = (char *)malloc((size + 1) * sizeof(char));
  if (!buffer) {
    fprintf(stderr, "ERROR: failed to allocate %zu bytes.\n", size + 1);
    exit(EXIT_FAILURE);
  }
  const size_t n_read = fread(buffer, sizeof(char), size, fp);
  if (n_read != size) {
    fprintf(stderr, "ERROR: Could not read file.\n");
    exit(EXIT_FAILURE);
  }
  buffer[size] = '\0';
  fseek(fp, position, SEEK_SET);
  return buffer;
}

bool next_line(Textfile *f) {
  return (getline(&f->line, &f->len, f->fp) != -1);
}

void overwrite_line(Textfile *f, const char *newline) {
  const size_t len = strlen(f->line);
  size_t newlen = strlen(newline) + 1;
  if (newlen <= len) {
    // New line has the same length or is shorter than the current one
    // one can simply overwrite the corresponding chars
    fseek(f->fp, -len, SEEK_CUR);
    fputs(newline, f->fp);
    // and add some padding if needed
    while (newlen < len) {
      fputs(" ", f->fp);
      newlen++;
    }
    fputs("\n", f->fp);
  } else {
    // New line is longer than the current one
    // We have to copy the rest of the file
    char *file_tail = file_to_buffer(f->fp);
    fseek(f->fp, -len, SEEK_CUR);
    fputs(newline, f->fp);
    fputs("\n", f->fp);
    fputs(file_tail, f->fp);
    free(file_tail);
  }
}

void write_line(Textfile *f, const char *line) {
  fputs(line, f->fp);
  fputs("\n", f->fp);
}

void file_close(Textfile *f) {
  fclose(f->fp);
  if (f->line) {
    free(f->line);
  }
  free(f);
}
