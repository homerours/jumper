#include <ctype.h>
#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

// #define ANSI_COLOR_RED "\x1b[31m"
// #define ANSI_COLOR_GREEN "\x1b[32m"
// #define ANSI_COLOR_YELLOW "\x1b[33m"
// #define ANSI_COLOR_BLUE "\x1b[34m"
// #define ANSI_COLOR_MAGENTA "\x1b[35m"
// #define ANSI_COLOR_CYAN "\x1b[36m"
// #define ANSI_COLOR_RESET "\x1b[0m"
static inline int max(int x, int y) { return ((x) > (y) ? x : y); }
static inline int min(int x, int y) { return ((x) < (y) ? x : y); }

typedef struct matching_data {
  char *string;
  char *query;
  int *matrix;
  int n;
  int m;
  int *char_bonus;
} matching_data;

const int gap_penalty = 1;
const int first_gap_penalty = 3;
const int match_base_reward = 8;
const int uppercase_bonus = 3;
const int start_bonus = 2;
const int end_of_path_bonus = 1;

int matching_value(char c) {
  if (isalpha(c)) {
    return 0;

  } else if (isdigit(c)) {
    return 0;
  }
  if (c == '/' || c == '_' || c == '-' || c == '.' || c == '#' || c == '\\') {
    return 1;
  }
  return 0;
}

int *matching_bonus(char *string, int n) {
  int *bonus = (int *)malloc(n * sizeof(int));
  int bonus_next_char = 0;
  int last_slash = -1;
  for (int i = 0; i < n; i++) {
    bonus[i] = bonus_next_char;
    bonus_next_char = matching_value(string[i]);
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

int score(char a, char b, int bonus) {
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
  int *matrix = (int *)malloc(n * m * sizeof(int));
  for (int i = 0; i <= n; i++) {
    matrix[i * m] = 0;
  }
  int N = min(n, m);
  for (int j = 1; j <= N - 1; j++) {
    for (int i = j - 1; i < n; i++) {
      matrix[i * m + j] = -1;
    }
  }
  data->n = n;
  data->m = m;
  data->string = string;
  data->query = query;
  data->matrix = matrix;
  data->char_bonus = matching_bonus(string, n);
  return data;
}
void free_matching_data(matching_data *data) {
  free(data->matrix);
  free(data->char_bonus);
  free(data);
}
// returns true if the parent of (i,j) is (i-1,j-1)
bool diag(matching_data *data, int i, int j, int *c) {
  int m = data->m;
  if (data->matrix[(i - 1) * m + j - 1] < 0) {
    return false;
  }
  *c = score(data->query[j - 1], data->string[i - 1], data->char_bonus[i - 1]);
  if (data->matrix[(i - 1) * m + j] < 0) {
    return true;
  }
  if (*c > 0) {
    return data->matrix[(i - 1) * m + j] - gap_penalty <=
           data->matrix[(i - 1) * m + j - 1] + *c;
  }
  return false;
}

char *extract_matching(matching_data *data, int imax) {
  int n = data->n, m = data->m;
  int *matrix = data->matrix;
  int i = imax, j = m - 1, c;
  int nbreaks = 0;
  bool is_matched = true;
  int *breaks = (int *)malloc(m * sizeof(int));
  while (j > 0) {
    if (diag(data, i, j, &c)) {
      if (!is_matched) {
        breaks[nbreaks++] = i - 1;
      }
      j -= 1;
      is_matched = true;
    } else {
      if (is_matched) {
        breaks[nbreaks++] = i - 1;
        is_matched = false;
      }
    }
    i -= 1;
  }
  const char *COLOR_GREEN = "\x1b[32m"; // 5
  const char *COLOR_RESET = "\x1b[0m";  // 4
  int new_len = 9 + n + nbreaks * 9 + 1;
  char *new_string = (char *)malloc(new_len * sizeof(char));
  new_string[new_len] = '\0';
  int k = 0;
  for (; k < i; k++) {
    new_string[k] = data->string[k];
  }
  strncpy(new_string + k, COLOR_GREEN, 5);
  k += 5;
  int sk = i;
  for (; sk < imax; sk++) {
    new_string[k] = data->string[sk];
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
    new_string[k] = data->string[sk];
  }
  return new_string;
}

int match(char *string, char *query, bool colors, char **matched_string) {
  if (*query == '\0') {
    *matched_string = strdup(string);
    return 1;
  }
  matching_data *data = make_data(string, query);
  int n = data->n, m = data->m;
  // for (int i = 0; i < n - 1; i++) {
  //   printf("%c  %d\n", string[i], data->char_bonus[i]);
  // }
  int *matrix = data->matrix;
  int margin = 0, imax = 0, maximum = -1, c;
  for (int i = 1; i < n; i++) {
    int k = min(i - margin, m - 1);
    for (int j = 1; j <= k; j++) {
      if (diag(data, i, j, &c)) {
        // Parent is (i-1,j-1), which can not be -1
        if (c < 0) {
          margin = i - j + 1;
          break;
        }
        matrix[i * m + j] = matrix[(i - 1) * m + j - 1] + c;
        // printf("Match (%d,%d) on letter %c  val=%d  (c=%d)\n", i, j,
        //        string[i - 1], matrix[i * m + j], c);
        if (j == (m - 1) && matrix[i * m + j] >= maximum) {
          maximum = matrix[i * m + j];
          // printf("Max (%d,%d) on letter %c  val=%d\n", i, j, string[i - 1],
          //        maximum);
          imax = i;
        }
      } else {
        // Parent is (i-1,j): it is not -1
        if (diag(data, i - 1, j, &c)) {
          // We had a match at (i-1,j) -> extra gap penalty
          matrix[i * m + j] =
              max(matrix[(i - 1) * m + j] - first_gap_penalty, 0);
        } else {
          matrix[i * m + j] = max(matrix[(i - 1) * m + j] - gap_penalty, 0);
        }
        // printf("Skip (%d,%d) on letter %c  val=%d  (c=%d)\n", i, j,
        //        string[i - 1], matrix[i * m + j], c);
      }
    }
  }
  // printf("End of match: imax=%d  maxi=%d\n", imax, maximum);
  if (imax == 0) {
    return 0;
  }
  if (colors) {
    *matched_string = extract_matching(data, imax);
  } else {
    *matched_string = strdup(string);
  }
  free_matching_data(data);
  return maximum;
}

// int main() {
//   char *string = "lodocm";
//   char *query = "lom";
//   char *result = "NONE";
//   int maximum = match(string, query, true, &result);
//   printf("%s\n", result);
//   printf("Maximum: %d\n", maximum);
//   return 0;
// }

static double match1(char *string, char *query, int query_length) {
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
