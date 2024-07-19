#pragma once

#include <stdbool.h>

typedef enum SYNTAX {
  SYNTAX_extended,
  SYNTAX_fuzzy,
  SYNTAX_exact,
} SYNTAX;

// Token types
typedef enum TTYPE {
  TTYPE_start,
  TTYPE_end,
  TTYPE_exact,
  TTYPE_fuzzy,
} TTYPE;

typedef struct Token {
  char *token;
  int length;
  TTYPE type;
} Token;

typedef struct TokenArray {
  Token *start;
  Token *end;
  Token **tokens;
  int length;
} TokenArray;

typedef struct Query {
  char *query;
  bool *gap_allowed;
  int length;
  double alignment;
} Query;

// Array of queries
typedef struct Queries {
  Query *queries;
  int n;
} Queries;

void free_queries(Queries queries);
Query make_standard_query(const char *query, bool gap_allowed);
Queries make_extended_queries(const char *query, bool orderless);
