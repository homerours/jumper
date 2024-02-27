#include <ctype.h>
#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

static inline int max(int x, int y) { return ((x) > (y) ? x : y); }
static inline int min(int x, int y) { return ((x) < (y) ? x : y); }

typedef struct scores {
  int match_score;
  int gap_score;
} scores;

typedef struct matching_data {
  char *string;
  char *query;
  scores *matrix;
  int n;
  int m;
  int *char_bonus;
} matching_data;

const int gap_penalty = 2;
const int first_gap_penalty = 5;
const int gap_penalty_separator = 1;
const int first_gap_penalty_separator = 2;
const int match_base_reward = 10;
const int uppercase_bonus = 3;
const int start_bonus = 3;
const int end_of_path_bonus = 1;

bool is_separator(char c) {
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
    for (int i = last_slash; i < n; i++) {
      bonus[i] += end_of_path_bonus;
    }
  }
  return bonus;
}

int score(matching_data *data, int i, int j) {
  char a = data->query[j - 1];
  char b = data->string[i - 1];
  int bonus = data->char_bonus[i - 1];
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
  scores *matrix = (scores *)malloc(n * m * sizeof(struct scores));
  for (int i = 0; i < n; i++) {
    matrix[i * m].match_score = 0;
    matrix[i * m].gap_score = 0;
  }
  for (int j = 1; j < m; j++) {
    for (int i = j - 1; i < n; i++) {
      matrix[i * m + j].match_score = -1;
      matrix[i * m + j].gap_score = -1;
    }
  }
  data->n = n;
  data->m = m;
  data->string = string;
  data->query = query;
  data->matrix = matrix;
  data->char_bonus = matching_bonus(string, n - 1);
  return data;
}

void free_matching_data(matching_data *data) {
  free(data->matrix);
  free(data->char_bonus);
  free(data);
}

void fill(matching_data *data, int i, int j) {
  int m = data->m;
  scores *cell = data->matrix + i * m + j;
  scores *top = data->matrix + (i - 1) * m + j;
  scores *top_left = data->matrix + (i - 1) * m + j - 1;
  int g =
      max(top->gap_score - gap_penalty, top->match_score - first_gap_penalty);
  cell->gap_score = max(g, -1);

  if ((top->gap_score != -1 || top->match_score != -1) &&
      cell->gap_score == -1) {
    cell->gap_score = 0;
  }

  int match = score(data, i, j);
  if (match > 0) {
    int maxi = max(top_left->gap_score, top_left->match_score);
    if (maxi >= 0) {
      cell->match_score = maxi + match;
    } else {
      cell->match_score = -1;
    }
  }
}

void parent(matching_data *data, int i, int j, bool *skip) {
  int m = data->m;
  scores cell = data->matrix[i * m + j];
  if (*skip) {
    *skip =
        (cell.match_score - first_gap_penalty <= cell.gap_score - gap_penalty);
  } else {
    *skip = (cell.match_score < cell.gap_score);
  }
}

char *extract_matching(matching_data *data, int i0, char *full_string) {
  int offset = data->string - full_string;
  int n = data->n + offset, m = data->m;
  int imax = i0 + offset;
  int i = imax, j = m - 1, c;
  int nbreaks = 0;
  bool is_matched = true;
  int *breaks = (int *)malloc(2 * m * sizeof(int));
  bool skip = false;
  while (j > 0) {
    parent(data, i - offset, j, &skip);
    if (skip) {
      if (is_matched) {
        breaks[nbreaks++] = i - 1;
        is_matched = false;
      }
    } else {
      if (!is_matched) {
        breaks[nbreaks++] = i - 1;
        is_matched = true;
      }
      j = j - 1;
    }
    i = i - 1;
  }
  const char *COLOR_GREEN = "\x1b[32m"; // 5
  const char *COLOR_RESET = "\x1b[0m";  // 4
  int new_len = 9 + n + nbreaks * 9 + 1;
  char *string = full_string;
  char *new_string = (char *)malloc(new_len * sizeof(char));
  new_string[new_len - 1] = '\0';
  int k = 0;
  for (; k < i; k++) {
    new_string[k] = string[k];
  }
  strncpy(new_string + k, COLOR_GREEN, 5);
  k += 5;
  int sk = i;
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
  k += 4;
  for (; sk < n; k++, sk++) {
    new_string[k] = string[sk];
  }
  return new_string;
}

bool quick_match(char *string, char *query, int* start, int*end) {
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
  int margin = 0, imax = 0, maximum = -1;
  scores *cell;
  for (int i = 1; i < n; i++) {
    int k = min(i - margin, m - 1);
    for (int j = 1; j <= k; j++) {
      fill(data, i, j);
      cell = data->matrix + i * m + j;
      if (j == k && cell->match_score == -1 && cell->gap_score == -1) {
        margin = i - j + 1;
        break;
      }
      if (j == (m - 1) && cell->match_score >= maximum) {
        imax = max(i, imax);
        maximum = cell->match_score;
      }
    }
  }

  if (imax == 0) {
    *score = 0;
    free_matching_data(data);
    return string;
  }
  *score = maximum;
  char *match_string;
  if (colors) {
    match_string = extract_matching(data, imax, string);
  } else {
    match_string = strdup(string);
  }
  free_matching_data(data);
  return match_string;
}

// int main() {
//   // char *string = "lodocm";
//   // char *query = "lom";
//   char *string = "/s/leo/";
//   char *query = "leo";
//   int maxi;
//   char *result = match(string, query, true, &maxi);
//   printf("%s\n", result);
//   printf("Maximum: %d\n", maxi);
//   return 0;
// }

double match1(char *string, char *query, int query_length) {
  if (*query == '\0') {
    return 1.0;
  }
  char *t = string, *q = query, *tb, *qb;
  int consecutive = 0, max_consecutive = 0;
  while (*t != 0 && *q != 0) {
    if (tolower(*t) == tolower(*q)) {
      if (consecutive == 0) {
        tb = t - 1;
        qb = q - 1;
        // Backward search
        while (qb >= query && tolower(*tb) == tolower(*qb)) {
          tb--;
          qb--;
          consecutive++;
        }
      }
      q++;
      consecutive++;
      max_consecutive = max(max_consecutive, consecutive);
    } else {
      if (consecutive == 1) {
        // Discard single char matches
        q--;
      }
      consecutive = 0;
    }
    t++;
  }
  if (consecutive == 1 && query_length > 1) {
    // Discard single char matches
    q--;
  }
  double s = ((double)(q - query)) / ((double)query_length);
  if (s < 0.3) {
    return 0.0;
  }
  double score = s * max_consecutive;
  if (s == 1.0) {
    score *= 10;
    if (strchr(t, '/') == NULL) {
      score += 50 * consecutive;
    }
  }
  return score;
}
