#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "matching.h"
#include "query.h"

static const char COLOR_GREEN[] = "\x1b[32m";
static const char COLOR_RESET[] = "\x1b[0m";

static inline int max(int x, int y) { return ((x) > (y) ? x : y); }

typedef struct Scores {
  int match;
  int gap;
} Scores;

typedef struct Breaks {
  int *breaks;
  int nbreaks;
} Breaks;

typedef struct MatchingData {
  const char *string;
  const char *query;
  int n;
  int m;
  int imax;
  int *bonus;
  bool *gap_allowed;
  Scores *matrix;
  CASE_MODE case_mode;
} MatchingData;

// Bonuses
static const int match_bonus = 20;
static const int post_separator_bonus = 4;
static const int post_slash_bonus = 6;
static const int camelcase_bonus = 2;
static const int uppercase_bonus = 6;
static const int end_of_path_bonus = 4;
// Penalties
static const int first_gap_penalty = 16;
static const int gap_penalty = 1;

// For orderless queries
static const double alignment_scaling = 80.0;

static inline bool match_char(char a, char b, CASE_MODE case_mode) {
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
    bonus[i] = 0;
    if (i > 0 && islower(string[i - 1]) && isupper(string[i])) {
      bonus[i] += camelcase_bonus;
    }
    bool is_sep = is_separator(string[i]);
    if (is_sep) {
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
  if (match_char(b, a, data->case_mode)) {
    int bonus = data->bonus[i - 1];
    int score = match_bonus + bonus;
    if (isupper(b) && a == b) {
      score += uppercase_bonus;
    }
    if (j == 1 && bonus > 0) {
      score += 2;
    }
    return score;
  }
  return -1;
}

static MatchingData *make_data(const char *string, Query query,
                               CASE_MODE case_mode) {
  const int n = strlen(string) + 1;
  const int m = query.length + 1;
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
  data->query = query.query;
  data->gap_allowed = query.gap_allowed;
  data->matrix = matrix;
  data->bonus = matching_bonus(string, n - 1);
  data->case_mode = case_mode;
  data->imax = 0;
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
  if (!data->gap_allowed[j]) {
    scores->gap = -1;
  } else {
    int g = max(top->gap - gap_penalty, top->match - first_gap_penalty);
    scores->gap = max(g, -1);

    if ((top->gap != -1 || top->match != -1) && scores->gap == -1) {
      scores->gap = 0;
    }
  }
  const int mscore = match_score(data, i, j);
  if (mscore > 0 && (j > 1 || i == 1 || data->gap_allowed[0])) {
    const int max_score = max(top_left->gap, top_left->match);
    if (max_score >= 0) {
      scores->match = max_score + mscore;
    }
  }
  return scores;
}

static bool parent(MatchingData *data, int i, int j, bool skip) {
  Scores *scores = get_scores(data, i, j);
  if (skip) {
    return (scores->match - first_gap_penalty <= scores->gap - gap_penalty);
  } else {
    return (scores->match < scores->gap);
  }
}

static Breaks extract_breaks(MatchingData *data) {
  int i = data->imax, j = data->m - 1;
  Breaks b = {.nbreaks = 0, .breaks = (int *)malloc(2 * data->m * sizeof(int))};
  bool is_matched = true, skip = false;
  b.breaks[b.nbreaks++] = i - 1;
  while (j > 0) {
    skip = parent(data, i, j, skip);
    i--;
    if (skip) {
      if (is_matched) {
        b.breaks[b.nbreaks++] = i;
        is_matched = false;
      }
    } else {
      if (!is_matched) {
        b.breaks[b.nbreaks++] = i;
        is_matched = true;
      }
      j--;
    }
  }
  b.breaks[b.nbreaks++] = i - 1;
  return b;
}

static char *add_ansi_colors(MatchingData *data, Breaks b) {
  const int new_len = data->n + (b.nbreaks / 2) * 9 + 1;
  char *new_string = (char *)malloc(new_len * sizeof(char));
  int k = 0;
  if (b.nbreaks > 0 && b.breaks[b.nbreaks - 1] == -1) {
    strncpy(new_string + k, COLOR_GREEN, 5);
    k += 5;
    b.nbreaks--;
  }
  for (int sk = 0; sk < data->n; sk++) {
    new_string[k] = data->string[sk];
    k++;
    if (b.nbreaks > 0 && b.breaks[b.nbreaks - 1] == sk) {
      if (b.nbreaks % 2 == 0) {
        strncpy(new_string + k, COLOR_GREEN, 5);
        k += 5;
      } else {
        strncpy(new_string + k, COLOR_RESET, 4);
        k += 4;
      }
      b.nbreaks--;
    }
  }
  new_string[k] = '\0';
  free(b.breaks);
  return new_string;
}

static bool quick_match(const char *string, Query query, CASE_MODE case_mode) {
  const char *t = string;
  const char *q = query.query;
  while (*t != 0 && *q != 0) {
    if (match_char(*t, *q, case_mode)) {
      q++;
    }
    t++;
  }
  return (*q == 0);
}

static int get_max_score(MatchingData *data) {
  int score = -1;
  const int n = data->n;
  const int m = data->m;
  Scores *scores;
  for (int ii = 1; ii < n - m + 2; ii++) {
    int i = ii + m - 2;
    scores = get_scores(data, i, m - 1);
    if (scores->match >= score && (data->gap_allowed[m - 1] || i == n - 1)) {
      data->imax = i;
      score = scores->match;
    }
  }
  return score;
}

double match_accuracy(const char *string, Queries queries, bool colors,
                      char **output, CASE_MODE case_mode) {

  double best_score = 0.0;
  MatchingData *best_matching_data = NULL;
  for (int iquery = 0; iquery < queries.n; iquery++) {
    Query query = queries.queries[iquery];
    if (*query.query == 0) {
      *output = strdup(string);
      return 1;
    }
    if (quick_match(string, query, case_mode)) {
      MatchingData *data = make_data(string, query, case_mode);
      const int n = data->n;
      const int m = data->m;
      Scores *scores;
      int j, jmax = 0; // jmax: max accessible column
      for (int ii = 1; ii < n - m + 2; ii++) {
        for (j = 1; j < m; j++) {
          scores = compute_scores(data, ii + j - 1, j);
          if (scores->match == -1 && scores->gap == -1 && j >= jmax) {
            break;
          }
        }
        jmax = j - 1;
      }
      const int score = get_max_score(data);
      const double total_score = score + alignment_scaling * query.alignment;
      if ((score > -1) && (total_score > best_score)) {
        best_score = total_score;
        if (best_matching_data != NULL) {
          free(best_matching_data);
        }
        best_matching_data = data;
      } else {
        free_matching_data(data);
      }
    }
  }
  if (best_score == 0.0) {
    return 0;
  }
  // We have a match, allocate memory
  if (colors) {
    *output =
        add_ansi_colors(best_matching_data, extract_breaks(best_matching_data));
  } else {
    *output = strdup(string);
  }
  free_matching_data(best_matching_data);
  return best_score;
}
