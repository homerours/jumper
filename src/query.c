#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "permutations.h"
#include "query.h"

void free_query(Query query) {
  free(query.query);
  free(query.gap_allowed);
}

void free_queries(Queries queries) {
  for (int i = 0; i < queries.n; i++) {
    free_query(queries.queries[i]);
  }
}

static void free_tokenarray(TokenArray t) {
  for (int i = 0; i < t.length; i++) {
    free(t.tokens[i]);
  }
  free(t.tokens);
}

static void set_values(bool *array, int start, int length, bool value) {
  for (int i = start; i <= start + length - 1; i++) {
    array[i] = value;
  }
}

Query make_normal_query(const char *query, bool gap_allowed) {
  Query q;
  const int n = strlen(query);
  q.gap_allowed = (bool *)malloc((n + 1) * sizeof(bool));
  q.length = n;
  q.alignment = 1.0;
  q.query = strdup(query);
  q.gap_allowed[0] = true;
  q.gap_allowed[n] = true;
  set_values(q.gap_allowed, 1, n - 1, gap_allowed);
  return q;
}

static Token *make_token(const char *token) {
  Token *t = malloc(sizeof(Token));
  const int n = strlen(token);
  if (*token == '\'') {
    t->type = TTYPE_exact;
    t->token = (char *)malloc(n * sizeof(char));
    strncpy(t->token, token + 1, n - 1);
    t->token[n - 1] = '\0';
    t->length = n - 1;
  } else if (*token == '^') {
    t->type = TTYPE_start;
    t->token = (char *)malloc(n * sizeof(char));
    strncpy(t->token, token + 1, n - 1);
    t->token[n - 1] = '\0';
    t->length = n - 1;
  } else if (token[n - 1] == '$') {
    t->type = TTYPE_end;
    t->token = (char *)malloc(n * sizeof(char));
    strncpy(t->token, token, n - 1);
    t->token[n - 1] = '\0';
    t->length = n - 1;
  } else {
    t->type = TTYPE_fuzzy;
    t->token = strdup(token);
    t->length = n;
  }
  return t;
}

void printarray(TokenArray array) {
  if (array.start != NULL) {
    printf("Start: %s\n", array.start->token);
  }
  if (array.end != NULL) {
    printf("End: %s\n", array.end->token);
  }
  for (int i = 0; i < array.length; i++) {
    printf("%s\n", array.tokens[i]->token);
  }
}

static TokenArray parse(const char *query) {
  char *pch;
  char *str = strdup(query);
  const int max_tokens = 50;
  TokenArray array;
  array.tokens = (Token **)malloc(max_tokens * sizeof(Token *));
  array.length = 0;
  array.start = NULL;
  array.end = NULL;
  pch = strtok(str, " ");
  while (pch != NULL) {
    if (strlen(pch) > 1 || (*pch != '^' && *pch != '\'' && *pch != '$')) {
      Token *token = make_token(pch);
      if (token->type == TTYPE_start) {
        array.start = token;
      } else if (token->type == TTYPE_end) {
        array.end = token;
      } else {
        if (array.length < max_tokens) {
          array.tokens[array.length++] = token;
        } else {
          fprintf(stderr,
                  "Error: jumper does not accept more than %d tokens.\n",
                  max_tokens);
          exit(EXIT_FAILURE);
        }
      }
    }
    pch = strtok(NULL, " ");
  }
  free(str);
  return array;
}

void print_query(Query query) {
  int n = query.length;
  printf(" %s\n", query.query);
  for (int i = 0; i <= n; i++) {
    if (query.gap_allowed[i]) {
      printf("1");
    } else {
      printf("0");
    }
  }
  printf("\n");
}

Query make_query(TokenArray array, Permutation *p) {
  Query q;
  q.alignment =
      (p->n > 1) ? 2.0 * ((double)p->alignment) / (p->n * (p->n - 1)) : 1.0;
  int n = 0;
  for (int i = 0; i < array.length; i++) {
    n += array.tokens[i]->length;
  }
  if (array.start != NULL) {
    n += array.start->length;
  }
  if (array.end != NULL) {
    n += array.end->length;
  }
  q.length = n;
  q.query = (char *)malloc((n + 1) * sizeof(char));
  q.query[n] = '\0';
  q.gap_allowed = (bool *)malloc((n + 1) * sizeof(bool));
  char *pos = q.query;
  if (array.start != NULL) {
    strncpy(pos, array.start->token, array.start->length);
    pos += array.start->length;
    set_values(q.gap_allowed, 0, array.start->length, false);
  }
  for (int i = 0; i < array.length; i++) {
    Token *token = array.tokens[p->values[i]];
    strncpy(pos, token->token, token->length);
    q.gap_allowed[pos - q.query] = true;
    set_values(q.gap_allowed, pos - q.query + 1, token->length - 1,
               token->type == TTYPE_fuzzy);
    pos += token->length;
  }
  q.gap_allowed[pos - q.query] = true;
  if (array.end != NULL) {
    strncpy(pos, array.end->token, array.end->length);
    set_values(q.gap_allowed, pos - q.query + 1, array.end->length, false);
  }
  return q;
}

// For "orderless" queries with n tokens, we will try to match the n! possible
// permutations of these tokens.
// Since n will be small most of the time (n <= 4), this will not
// affect runtime significantly.
// For larger values of n (n=5,6,7), we we only consider permutations
// that are close enough (in the sense of having a certain number of
// pairs of token well ordered) to the identity.
Queries make_extended_queries(const char *query, bool orderless) {
  TokenArray array = parse(query);
  const int N = array.length;
  Permutation *p = init_permutation(N);
  Queries q;
  q.queries = (Query *)malloc(30 * sizeof(Query));
  q.queries[0] = make_query(array, p);
  q.n = 1;
  if (orderless && N <= 7) {
    double threshold = 0.0;
    // the following ensures that we always consider less than 30 permutations
    if (N == 5) {
      threshold = 0.7;
    } else if (N == 6) {
      threshold = 0.81;
    } else if (N == 7) {
      threshold = 0.9;
    }
    const double t = threshold * N * (N - 1) / 2.0;
    while (next_permutation(p)) {
      if (p->alignment >= t) {
        q.queries[q.n++] = make_query(array, p);
      }
    }
  }
  free_permutation(p);
  free_tokenarray(array);
  return q;
}
