#pragma once

#include <stdbool.h>
#include <string.h>

// Match a path against a single glob pattern
// Supports: * (any characters), ? (single character), [...] (character class)
bool glob_match(const char *pattern, const char *path);

// Check if path matches any pattern in the list
// Returns true if any pattern matches
bool glob_match_list(char **patterns, const char *path);

char **read_filters(const char *path);
void free_filters(char ** filters);
