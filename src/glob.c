#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "glob.h"
#include "textfile.h"

char **read_filters(const char *path) {
  if (!path) {
    return NULL;
  }
  Textfile *f = file_open(path);
  if (!f) {
    // If the filters' file does not exist, we do as if there were no filters
    return NULL;
  }
  int buffer_size = 20;
  char **filters = (char **)malloc(buffer_size * sizeof(char *));
  int i = 0;
  while (next_line(f)) {
    // Remove trailing newline
    f->line[strcspn(f->line, "\n")] = '\0';
    // Skip empty lines
    if (f->line[0] == '\0') {
      continue;
    }
    filters[i] = strdup(f->line);
    i++;
    if (i == buffer_size) {
      buffer_size = buffer_size * 2;
      filters = realloc(filters, buffer_size * sizeof(char *));
    }
  }
  filters[i] = NULL;
  file_close(f);
  return filters;
}
void free_filters(char **filters) {
  if (!filters)
    return;
  char **pointer = filters;
  while (*pointer) {
    free(*pointer);
    pointer++;
  }
  free(filters);
}

// Match a single character against a character class [...]
static bool match_char_class(const char *pattern, int *pattern_pos, char c) {
  int pos = *pattern_pos;
  bool match = false;
  bool negated = false;

  if (pattern[pos] == '^' || pattern[pos] == '!') {
    negated = true;
    pos++;
  }

  while (pattern[pos] && pattern[pos] != ']') {
    // Handle ranges like a-z
    if (pattern[pos + 1] == '-' && pattern[pos + 2] != ']' &&
        pattern[pos + 2] != '\0') {
      if (c >= pattern[pos] && c <= pattern[pos + 2]) {
        match = true;
      }
      pos += 3;
    } else {
      if (c == pattern[pos]) {
        match = true;
      }
      pos++;
    }
  }

  if (pattern[pos] == ']') {
    pos++;
  }

  *pattern_pos = pos;
  return negated ? !match : match;
}

static bool glob_match_internal(const char *pattern, const char *path, int p,
                                int s) {
  int star_p = -1, star_s = -1;

  while (path[s]) {
    // Check for ** pattern
    if (pattern[p] == '*' && pattern[p + 1] == '*') {
      // ** can match anything including slashes
      if (pattern[p + 2] == '/' || pattern[p + 2] == '\0') {
        // Valid ** pattern
        int next_p = pattern[p + 2] == '/' ? p + 3 : p + 2;

        // Try matching rest of pattern at each position
        // But only at path segment boundaries (start or after /)
        for (int i = s; path[i] != '\0'; i++) {
          // Only try to match at start of path segments
          if (i == s || path[i - 1] == '/') {
            if (glob_match_internal(pattern, path, next_p, i)) {
              return true;
            }
          }
        }
        // Also try matching from end
        return glob_match_internal(pattern, path, next_p, s + strlen(path + s));
      } else {
        // Treat ** as two single stars if not followed by / or end
        p++;
        star_p = p;
        star_s = s;
      }
    } else if (pattern[p] == '*') {
      // Single * doesn't match /
      star_p = p++;
      star_s = s;
    } else if (pattern[p] == '?') {
      // ? matches any single character except /
      if (path[s] == '/') {
        if (star_p != -1) {
          p = star_p + 1;
          s = ++star_s;
        } else {
          return false;
        }
      } else {
        p++;
        s++;
      }
    } else if (pattern[p] == '[') {
      // Character class
      p++;
      if (match_char_class(pattern, &p, path[s])) {
        s++;
      } else if (star_p != -1) {
        p = star_p + 1;
        s = ++star_s;
      } else {
        return false;
      }
    } else if (pattern[p] == path[s]) {
      // Exact character match
      p++;
      s++;
    } else if (star_p != -1) {
      // No match, backtrack to last *
      // Single * doesn't match /
      if (path[star_s] == '/') {
        return false;
      }
      p = star_p + 1;
      s = ++star_s;
    } else {
      return false;
    }
  }

  // Skip any trailing * or ** in pattern
  while (pattern[p] == '*') {
    if (pattern[p + 1] == '*') {
      p += 2;
      if (pattern[p] == '/')
        p++;
    } else {
      p++;
    }
  }

  return pattern[p] == '\0';
}

// Match a path against a single glob pattern
bool glob_match(const char *pattern, const char *path) {
  return glob_match_internal(pattern, path, 0, 0);
}

// Check if path matches any pattern in the list
bool glob_match_list(char **patterns, const char *path) {
  if (!patterns) {
    return false;
  }
  char **pattern = patterns;
  while (*pattern) {
    if (glob_match(*pattern, path)) {
      return true;
    }
    pattern++;
  }
  return false;
}
