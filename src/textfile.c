#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "textfile.h"

Textfile *file_open(const char *path) {
  Textfile *data = (Textfile *)malloc(sizeof(Textfile));
  data->fp = fopen(path, "r");
  if (!data->fp) {
    fprintf(stderr, "ERROR: File %s not found\n", path);
    exit(EXIT_FAILURE);
  }
  data->line = NULL;
  return data;
}
Textfile *file_open_rw(const char *path) {
  Textfile *data = (Textfile *)malloc(sizeof(Textfile));
  data->fp = fopen(path, "r+");
  data->position = ftell(data->fp);
  if (!data->fp) {
    // Textfile does not exist, we create it.
    data->fp = fopen(path, "w+");
    if (!data->fp) {
      fprintf(stderr, "ERROR: Couldn't open file %s.\n", path);
      exit(EXIT_FAILURE);
    }
  }
  data->line = NULL;
  return data;
}

static char *file_to_buffer(FILE *fp) {
  long int position = ftell(fp);
  fseek(fp, 0, SEEK_END);
  long int end_position = ftell(fp);
  fseek(fp, position, SEEK_SET);
  size_t size = end_position - position;
  char *buffer = (char *)malloc((size + 1) * sizeof(char));
  if (!buffer) {
    fprintf(stderr, "ERROR: failed to allocate %zu bytes.\n", size + 1);
    exit(EXIT_FAILURE);
  }
  size_t n_read = fread(buffer, sizeof(char), size, fp);
  if (n_read != size) {
    fprintf(stderr, "ERROR: Could not read file.\n");
    exit(EXIT_FAILURE);
  }
  buffer[size] = '\0';
  return buffer;
}

bool next_line(Textfile *data) {
  size_t len;
  data->position = ftell(data->fp);
  return (getline(&data->line, &len, data->fp) != -1);
}

void overwrite_line(Textfile *data, const char *newline) {
  int len = strlen(newline);
  long int new_position = ftell(data->fp);
  if (len <= new_position - data->position - 1) {
    // New line has the same length or is shorter than the current one
    // one can simply overwrite the corresponding chars
    fseek(data->fp, data->position, SEEK_SET);
    fputs(newline, data->fp);
    // and add some padding if needed
    while (len < new_position - data->position - 1) {
      fputs(" ", data->fp);
      len++;
    }
  } else {
    // New line is longer than the current one
    // We have to copy the rest of the file
    fseek(data->fp, new_position - 1, SEEK_SET);
    char *file_tail = file_to_buffer(data->fp);
    fseek(data->fp, data->position, SEEK_SET);
    fputs(newline, data->fp);
    fputs(file_tail, data->fp);
    free(file_tail);
  }
}

void write_line(Textfile *data, const char *line) {
  fputs(line, data->fp);
  fputs("\n", data->fp);
}

void file_close(Textfile *data) {
  fclose(data->fp);
  if (data->line) {
    free(data->line);
  }
  free(data);
}
