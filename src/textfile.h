#include <stdbool.h>

typedef struct Textfile {
  char *line;
  FILE *fp;
  long int position;
} Textfile;

Textfile *file_open(const char *path);
Textfile *file_open_rw(const char *path);
bool next_line(Textfile *data);
void overwrite_line(Textfile *data, const char *newline);
void write_line(Textfile *data, const char *line);
void file_close(Textfile *data);
