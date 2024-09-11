#include <stdbool.h>
#include <stdio.h>

typedef struct Textfile {
  char *line;
  size_t len;
  FILE *fp;
} Textfile;

Textfile *file_open(const char *path);
Textfile *file_open_rw(const char *path);
bool next_line(Textfile *f);
void overwrite_line(Textfile *f, const char *newline);
void write_line(Textfile *f, const char *line);
void file_close(Textfile *f);
