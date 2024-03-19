#include <ctype.h>
#include <getopt.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "arguments.h"
#include "heap.h"

static const char HELP_STRING[] =
    "Usage: %s [OPTIONS] QUERY\n"
    " -f, --file=FILE_PATH      Path to the database file.\n"
    " -a, --add                 Add the query to the database or\n"
    "                           update its record.\n"
    " -n, --n-results=N         Maximum number of results to show.\n"
    " -c, --color               Highlight matches in outputs.\n"
    " -s, --scores              Print the scores of the matches.\n"
    " -b, --beta=BETA           Specify an inverse temperature\n"
    "                           when computing the score (default=1.0).\n"
    " -h, --help                Display this help and exit\n";

static void help(const char *argv0) { printf(HELP_STRING, argv0); }

static struct option longopts[] = {
    {"file", required_argument, NULL, 'f'},
    {"scores", no_argument, NULL, 's'},
    {"add", no_argument, NULL, 'a'},
    {"color", no_argument, NULL, 'c'},
    {"beta", optional_argument, NULL, 'b'},
    {"n-results", optional_argument, NULL, 'n'},
    {"help", optional_argument, NULL, 'h'},
    {NULL, 0, NULL, 0}};

void args_init(Arguments *args) {
  args->file_path = NULL;
  args->key = "";
  args->n_results = MAX_HEAP_SIZE;
  args->highlight = false;
  args->print_scores = false;
  args->mode = MODE_search;
  args->beta = 1.0;
}

Arguments *parse_arguments(int argc, char **argv) {

  Arguments *args = (Arguments *)malloc(sizeof(Arguments));
  args_init(args);

  if (argc == 1) {
    help(argv[0]);
    exit(EXIT_SUCCESS);
  }
  int c;
  while ((c = getopt_long(argc, argv, "cashn:f:m:", longopts, NULL)) != -1) {
    switch (c) {
    case 'c':
      args->highlight = true;
      break;
    case 's':
      args->print_scores = true;
      break;
    case 'n':
      args->n_results = atoi(optarg);
      if (args->n_results <= 0) {
        fprintf(stderr,
                "ERROR: The number of results -n has to be positive!\n");
        exit(EXIT_FAILURE);
      }
      break;
    case 'a':
      args->mode = MODE_add;
      break;
    case 'f':
      args->file_path = optarg;
      break;
    case 'm':
      args->beta = atof(optarg);
      break;
    case 'h':
      help(argv[0]);
      exit(EXIT_SUCCESS);
      break;
    default:
      help(argv[0]);
      exit(EXIT_SUCCESS);
    }
  }
  if (args->file_path == NULL) {
    fprintf(stderr, "ERROR: you must provide a data file with -f.\n");
    exit(EXIT_FAILURE);
  }
  if (optind < argc - 1) {
    help(argv[0]);
    exit(EXIT_FAILURE);
  }
  if (optind != argc) {
    args->key = argv[argc - 1];
  }
  if (*(args->key) == 0 && args->mode == MODE_add) {
    fprintf(stderr, "ERROR: no query.\n");
    exit(EXIT_FAILURE);
  }
  return args;
}
