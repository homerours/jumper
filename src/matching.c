#include <ctype.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "matching.h"

static const char COLOR_GREEN[] = "\x1b[32m";
static const char COLOR_RESET[] = "\x1b[0m";

static inline int max(int x, int y) { return ((x) > (y) ? x : y); }

typedef struct Scores {
  int match;
  int gap;
} Scores;

typedef struct MatchingData {
  const char *string;
  const char *query;
  int n;
  int m;
  int *bonus;
  Scores *matrix;
  enum CASE_MODE case_mode;
} MatchingData;

// Bonuses
static const int match_bonus = 10;
static const int post_separator_bonus = 2;
static const int post_slash_bonus = 3;
static const int uppercase_bonus = 3;
static const int separator_bonus = 3;
static const int end_of_path_bonus = 2;
// Penalties
static const int first_gap_penalty = 9;
static const int gap_penalty = 1;

static inline bool match_char(char a, char b, enum CASE_MODE case_mode) {
  if (tolower(a) != tolower(b)) {
    return false;
  }
  if (case_mode == CASE_MODE_insensitive) {
    return true;
  }
  if (case_mode == CASE_MODE_sensitive) {
    return (a == b);
  }
  // Default, semi_sensitive mode
  return islower(b) || (a == b);
}

static inline bool is_separator(char c) {
  return (c == '/' || c == '_' || c == '-' || c == '.' || c == '#' ||
          c == '\\' || c == ' ');
}

static int *matching_bonus(const char *string, int n) {
  int *bonus = (int *)malloc(n * sizeof(int));
  int last_slash = -1;
  bool prev_is_sep = true;
  for (int i = 0; i < n; i++) {
    bonus[i] = match_bonus;
    bool is_sep = is_separator(string[i]);
    if (is_sep) {
      bonus[i] += separator_bonus;
      if (string[i] == '/') {
        last_slash = i;
      }
    } else if (prev_is_sep) {
      if (i > 0 && string[i - 1] == '/') {
        bonus[i] += post_slash_bonus;
      } else {
        bonus[i] += post_separator_bonus;
      }
    }
    prev_is_sep = is_sep;
  }
  if (last_slash > 0) {
    for (int i = last_slash + 1; i < n; i++) {
      bonus[i] += end_of_path_bonus;
    }
  }
  return bonus;
}

static int match_score(MatchingData *data, int i, int j) {
  const char a = data->query[j - 1];
  char b = data->string[i - 1];
  int bonus = data->bonus[i - 1];
  if (match_char(b, a, data->case_mode)) {
    if (isupper(b) && a == b) {
      return bonus + uppercase_bonus;
    }
    return bonus;
  }
  return -1;
}

static MatchingData *make_data(const char *string, const char *query,
                               enum CASE_MODE case_mode) {
  const int n = strlen(string) + 1;
  const int m = strlen(query) + 1;
  const int h = n - m + 2;
  Scores *matrix = (Scores *)malloc(h * m * sizeof(struct Scores));
  for (int i = 0; i < h; i++) {
    matrix[i * m].match = 0;
    matrix[i * m].gap = 0;
    for (int j = 1; j < m; j++) {
      matrix[i * m + j].match = -1;
      matrix[i * m + j].gap = -1;
    }
  }

  MatchingData *data = (MatchingData *)malloc(sizeof(MatchingData));
  data->n = n;
  data->m = m;
  data->string = string;
  data->query = query;
  data->matrix = matrix;
  data->bonus = matching_bonus(string, n - 1);
  data->case_mode = case_mode;
  return data;
}

static inline Scores *get_scores(MatchingData *data, int i, int j) {
  return data->matrix + (i - j + 1) * data->m + j;
}

static void free_matching_data(MatchingData *data) {
  free(data->matrix);
  free(data->bonus);
  free(data);
}

static Scores *compute_scores(MatchingData *data, int i, int j) {
  Scores *scores = get_scores(data, i, j);
  Scores *top = get_scores(data, i - 1, j);
  Scores *top_left = get_scores(data, i - 1, j - 1);
  int g = max(top->gap - gap_penalty, top->match - first_gap_penalty);
  scores->gap = max(g, -1);

  if ((top->gap != -1 || top->match != -1) && scores->gap == -1) {
    scores->gap = 0;
  }

  int mscore = match_score(data, i, j);
  if (mscore > 0) {
    int max_score = max(top_left->gap, top_left->match);
    if (max_score >= 0) {
      scores->match = max_score + mscore;
    }
  }
  return scores;
}

static void parent(MatchingData *data, int i, int j, bool *skip) {
  Scores *scores = get_scores(data, i, j);
  if (*skip) {
    *skip = (scores->match - first_gap_penalty <= scores->gap - gap_penalty);
  } else {
    *skip = (scores->match < scores->gap);
  }
}

static char *extract_matching(MatchingData *data, int imax) {
  const int n = data->n;
  int i = imax, j = data->m - 1;
  int nbreaks = 0;
  int *breaks = (int *)malloc(2 * data->m * sizeof(int));
  bool is_matched = true;
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
  const char *string = data->string;
  const int new_len = 9 + n + nbreaks * 9 + 1;
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

static bool quick_match(const char *string, const char *query,
                        enum CASE_MODE case_mode) {
  const char *t = string;
  const char *q = query;
  while (*t != 0 && *q != 0) {
    if (match_char(*t, *q, case_mode)) {
      q++;
    }
    t++;
  }
  return *q == 0;
}

int match_accuracy(const char *string, const char *query, bool colors,
                   char **matched_string, enum CASE_MODE case_mode) {
  if (*query == 0) {
    *matched_string = strdup(string);
    return 1;
  }
  if (!quick_match(string, query, case_mode)) {
    *matched_string = (char *)string;
    return 0;
  }
  MatchingData *data = make_data(string, query, case_mode);
  const int n = data->n;
  const int m = data->m;
  const int h = n - m + 2;
  int imax = 0, maximum = -1, i;
  Scores *scores;
  for (int ii = 1; ii < h; ii++) {
    for (int j = 1; j < m; j++) {
      i = ii + j - 1;
      scores = compute_scores(data, i, j);
      if (scores->match == -1 && scores->gap == -1) {
        break;
      }
      if (j == (m - 1) && scores->match >= maximum) {
        imax = i;
        maximum = scores->match;
      }
    }
  }
  if (maximum == -1) {
    free_matching_data(data);
    *matched_string = (char *)string;
    return 0;
  }
  // We have a match, allocate memory
  if (colors) {
    *matched_string = extract_matching(data, imax);
  } else {
    *matched_string = strdup(string);
  }
  free_matching_data(data);
  return maximum;
}
