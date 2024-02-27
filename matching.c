#include <ctype.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

static inline int max(int x, int y) { return ((x) > (y) ? x : y); }
static inline int min(int x, int y) { return ((x) < (y) ? x : y); }

typedef struct scores {
  int match;
  int gap;
} scores;

typedef struct matching_data {
  char *string;
  char *query;
  int n;
  int m;
  int *bonus;
  scores *matrix;
} matching_data;

const int gap_penalty = 2;
const int first_gap_penalty = 5;
const int gap_penalty_separator = 1;
const int first_gap_penalty_separator = 2;
const int match_base_reward = 10;
const int uppercase_bonus = 3;
const int start_bonus = 3;
const int end_of_path_bonus = 2;

static inline bool is_separator(char c) {
  return (c == '/' || c == '_' || c == '-' || c == '.' || c == '#' ||
          c == '\\' || c == ' ');
}

int *matching_bonus(char *string, int n) {
  int *bonus = (int *)malloc(n * sizeof(int));
  int last_slash = -1;
  bonus[0] = start_bonus;

  for (int i = 1; i < n; i++) {
    if (is_separator(string[i - 1]) && isalpha(string[i])) {
      bonus[i] = start_bonus;
    } else {
      bonus[i] = 0;
    }
    if (string[i] == '/') {
      last_slash = i;
    }
  }
  if (last_slash > 0) {
    for (int i = last_slash + 1; i < n; i++) {
      bonus[i] += end_of_path_bonus;
    }
  }
  return bonus;
}

int score(matching_data *data, int i, int j) {
  char a = data->query[j - 1];
  char b = data->string[i - 1];
  int bonus = data->bonus[i - 1];
  if (isupper(b) && a == b) {
    return match_base_reward + bonus + uppercase_bonus;
  }
  if (tolower(a) == tolower(b)) {
    return match_base_reward + bonus;
  }
  return -1;
}

matching_data *make_data(char *string, char *query) {
  matching_data *data = (matching_data *)malloc(sizeof(struct matching_data));
  int n = strlen(string) + 1;
  int m = strlen(query) + 1;
  int h = n - m + 2;
  scores *matrix = (scores *)malloc(h * m * sizeof(struct scores));

  for (int i = 0; i < h; i++) {
    matrix[i * m].match = 0;
    matrix[i * m].gap = 0;
    for (int j = 1; j < m; j++) {
      matrix[i * m + j].match = -1;
      matrix[i * m + j].gap = -1;
    }
  }
  data->n = n;
  data->m = m;
  data->string = string;
  data->query = query;
  data->matrix = matrix;
  data->bonus = matching_bonus(string, n - 1);
  return data;
}

static inline scores *get_cell(matching_data *data, int i, int j) {
  return data->matrix + (i - j + 1) * data->m + j;
}

void free_matching_data(matching_data *data) {
  free(data->matrix);
  free(data->bonus);
  free(data);
}

scores *fill(matching_data *data, int i, int j) {
  scores *cell = get_cell(data, i, j);
  scores *top = get_cell(data, i - 1, j);
  scores *top_left = get_cell(data, i - 1, j - 1);
  int g = max(top->gap - gap_penalty, top->match - first_gap_penalty);
  cell->gap = max(g, -1);

  if ((top->gap != -1 || top->match != -1) && cell->gap == -1) {
    cell->gap = 0;
  }

  int match = score(data, i, j);
  if (match > 0) {
    int maxi = max(top_left->gap, top_left->match);
    if (maxi >= 0) {
      cell->match = maxi + match;
    }
  }
  return cell;
}

void parent(matching_data *data, int i, int j, bool *skip) {
  scores *cell = get_cell(data, i, j);
  if (*skip) {
    *skip = (cell->match - first_gap_penalty <= cell->gap - gap_penalty);
  } else {
    *skip = (cell->match < cell->gap);
  }
}

char *extract_matching(matching_data *data, int imax) {
  int n = data->n, m = data->m;
  int i = imax, j = m - 1;
  int nbreaks = 0;
  bool is_matched = true;
  int *breaks = (int *)malloc(2 * m * sizeof(int));
  bool skip = false;
  while (j > 0) {
    parent(data, i, j, &skip);
    i--;
    if (skip) {
      if (is_matched) {
        breaks[nbreaks++] = i;
        is_matched = false;
      }
    } else {
      if (!is_matched) {
        breaks[nbreaks++] = i;
        is_matched = true;
      }
      j--;
    }
  }
  const char *COLOR_GREEN = "\x1b[32m"; // 5
  const char *COLOR_RESET = "\x1b[0m";  // 4
  char *string = data->string;
  int new_len = 9 + n + nbreaks * 9 + 1;
  char *new_string = (char *)malloc(new_len * sizeof(char));
  new_string[new_len - 1] = '\0';
  strncpy(new_string, string, i);
  strncpy(new_string + i, COLOR_GREEN, 5);
  int k = i + 5, sk = i;
  for (; sk < imax; sk++) {
    new_string[k] = string[sk];
    k++;
    if (nbreaks > 0 && breaks[nbreaks - 1] == sk) {
      if (nbreaks % 2 == 1) {
        strncpy(new_string + k, COLOR_GREEN, 5);
        k += 5;
      } else {
        strncpy(new_string + k, COLOR_RESET, 4);
        k += 4;
      }
      nbreaks--;
    }
  }
  free(breaks);
  strncpy(new_string + k, COLOR_RESET, 4);
  strncpy(new_string + k + 4, string + sk, n - sk);
  return new_string;
}

bool quick_match(char *string, char *query, int *start, int *end) {
  char *t = string, *q = query;
  while (*t != 0 && *q != 0) {
    if (tolower(*t) == tolower(*q)) {
      if (q == query) {
        *start = t - string;
      }
      *end = t - string;
      q++;
    }
    t++;
  }
  return *q == 0;
}

char *match(char *string, char *query, bool colors, int *score) {
  if (*query == '\0') {
    *score = 1;
    return strdup(string);
  }
  int start, end;
  if (!quick_match(string, query, &start, &end)) {
    *score = 0;
    return string;
  }
  matching_data *data = make_data(string, query);
  int n = data->n, m = data->m;
  int h = n - m + 2, i;
  int imax = 0, maximum = -1;
  scores *cell;
  for (int ii = 1; ii < h; ii++) {
    for (int j = 1; j < m; j++) {
      i = ii + j - 1;
      cell = fill(data, i, j);
      if (cell->match == -1 && cell->gap == -1) {
        break;
      }
      if (j == (m - 1) && cell->match >= maximum) {
        imax = max(i, imax);
        maximum = cell->match;
      }
    }
  }
  if (imax == 0) {
    *score = 0;
    free_matching_data(data);
    return string;
  }
  *score = maximum;
  // We have a match, allocate memory
  char *match_string;
  if (colors) {
    match_string = extract_matching(data, imax);
  } else {
    match_string = strdup(string);
  }
  free_matching_data(data);
  return match_string;
}
