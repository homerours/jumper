#include <ctype.h>
#include <getopt.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "arguments.h"
#include "shell.h"

static const char HELP_STRING[] =
    "Usage: %s [OPTIONS] QUERY\n"
    " -f, --file=FILE_PATH      Path to the database file.\n"
    " -h, --help                Display this help and exit.\n\n"
    "Search:\n"
    " -n, --n-results=N         Maximum number of results to show.\n"
    " -c, --color               Highlight matches in outputs.\n"
    " -s, --scores              Print the scores of the matches.\n"
    " -b, --beta=BETA           Specify an inverse temperature\n"
    "                           when computing the score (default=1.0).\n"
    " -x, --syntax=syntax       Query syntax (default: extended).\n"
    " -o, --orderless           Orderless queries: token can be matched\n"
    "                           in any order (only for extended syntax).\n"
    " -I, --case-insensitive    Make the search case-insenstitive.\n"
    " -S, --case-sensitive      Make the search case-senstitive.\n"
    " -H, --home-tilde          Substitute $HOME with ~ when printing "
    "results.\n"
    " -r, --relative=PATH       Outputs relative paths to PATH if\n"
    "                           specified (defaults to current directory).\n\n"
    "Update the database:\n"
    " -a, --add                 Add the query to the database or\n"
    "                           update its record.\n"
    " -w, --weight=WEIGHT       Weight of the visit (default=1.0).\n"
    " -C, --clean=TYPE          Remove from the database the entries \n"
    "                           that do not exist in the filesystem.\n"
    "                           Set TYPE to f for files, d for directories.\n\n"
    "Shell setup:\n"
    " -l, --shell=SHELL         Prints the setup script for SHELL,\n"
    "                           SHELL has to be bash, zsh or fish.\n";

static void help(const char *argv0) { printf(HELP_STRING, argv0); }

static struct option longopts[] = {
    {"file", required_argument, NULL, 'f'},
    {"add", no_argument, NULL, 'a'},
    {"weight", required_argument, NULL, 'w'},
    {"scores", no_argument, NULL, 's'},
    {"color", no_argument, NULL, 'c'},
    {"home-tilde", no_argument, NULL, 'H'},
    {"case-insenstitive", no_argument, NULL, 'I'},
    {"case-sensitive", no_argument, NULL, 'S'},
    {"relative", optional_argument, NULL, 'r'},
    {"beta", required_argument, NULL, 'b'},
    {"n-results", required_argument, NULL, 'n'},
    {"syntax", required_argument, NULL, 'x'},
    {"orderless", no_argument, NULL, 'o'},
    {"help", no_argument, NULL, 'h'},
    {"shell", required_argument, NULL, 'l'},
    {"clean", required_argument, NULL, 'C'},
    {NULL, 0, NULL, 0}};

static void args_init(Arguments *args) {
  args->file_path = NULL;
  args->key = "";
  args->n_results = 50000;
  args->highlight = false;
  args->print_scores = false;
  args->home_tilde = false;
  args->orderless = false;
  args->is_dir = true;
  args->relative_to = NULL;
  args->mode = MODE_search;
  args->syntax = SYNTAX_extended;
  args->case_mode = CASE_MODE_semi_sensitive;
  // frecency(path visited every day) ~ frecency(path visited every two days) +
  // 1 a beta of 1.0 makes this correspond to a difference of +2 in matching
  // score.
  args->beta = 1.0;
  args->weight = 1.0;
}

static SYNTAX parse_syntax(const char *syntax) {
  if (strcmp(syntax, "extended") == 0) {
    return SYNTAX_extended;
  } else if (strcmp(syntax, "fuzzy") == 0) {
    return SYNTAX_fuzzy;
  } else if (strcmp(syntax, "exact") == 0) {
    return SYNTAX_exact;
  }
  fprintf(stderr, "ERROR: Invalid argument for -x (--syntax): %s\n", syntax);
  fprintf(stderr, "Accepted arguments: extended, fuzzy, exact.\n");
  exit(EXIT_FAILURE);
}

Arguments *parse_arguments(int argc, char **argv) {

  Arguments *args = (Arguments *)malloc(sizeof(Arguments));
  args_init(args);

  if (argc == 1) {
    help(argv[0]);
    exit(EXIT_SUCCESS);
  }
  int c;
  while ((c = getopt_long(argc, argv, "cashoHISl:f:n:w:b:x:r::C:", longopts,
                          NULL)) != -1) {
    switch (c) {
    case 'f':
      args->file_path = optarg;
      break;
    case 'a':
      args->mode = MODE_update;
      break;
    case 'C':
      if ( (strlen(optarg) != 1 || (*optarg != 'f' && *optarg != 'd'))) {
        fprintf(stderr, "ERROR: Invalid argument for -C (--clean): %s\n",
                optarg);
        help(argv[0]);
        exit(EXIT_FAILURE);
      }
      args->is_dir = (*optarg == 'd');
      args->mode = MODE_clean;
      break;
    case 'I':
      args->case_mode = CASE_MODE_insensitive;
      break;
    case 'S':
      args->case_mode = CASE_MODE_sensitive;
      break;
    case 'c':
      args->highlight = true;
      break;
    case 'H':
      args->home_tilde = true;
      break;
    case 'x':
      args->syntax = parse_syntax(optarg);
      break;
    case 'o':
      args->orderless = true;
      break;
    case 's':
      args->print_scores = true;
      break;
    case 'r':
      if (optarg == NULL && optind < argc && argv[optind][0] == '/') {
        optarg = argv[optind++];
      }
      if (optarg == NULL) {
        const size_t max_cwd_len = 256;
        char *cwd = (char *)malloc(max_cwd_len * sizeof(char));
        args->relative_to = getcwd(cwd, max_cwd_len);
      } else {
        args->relative_to = optarg;
      }
      break;
    case 'n':
      if (sscanf(optarg, "%d", &args->n_results) != 1) {
        fprintf(stderr, "ERROR: Invalid argument for -n (--n-results): %s\n",
                optarg);
        help(argv[0]);
        exit(EXIT_FAILURE);
      }
      if (args->n_results <= 0) {
        fprintf(stderr,
                "ERROR: The number of results -n has to be positive.\n");
        exit(EXIT_FAILURE);
      }
      break;
    case 'b':
      if (sscanf(optarg, "%lf", &args->beta) != 1) {
        fprintf(stderr, "ERROR: Invalid argument for -b (--beta): %s\n",
                optarg);
        help(argv[0]);
        exit(EXIT_FAILURE);
      }
      break;
    case 'w':
      if (sscanf(optarg, "%lf", &args->weight) != 1) {
        fprintf(stderr, "ERROR: Invalid argument for -w (--weight): %s\n",
                optarg);
        help(argv[0]);
        exit(EXIT_FAILURE);
      }
      if (args->weight < 0) {
        fprintf(
            stderr,
            "ERROR: The weight of a visit (-w option) can't be negative.\n");
        exit(EXIT_FAILURE);
      }
      break;
    case 'h':
      help(argv[0]);
      exit(EXIT_SUCCESS);
      break;
    case 'l':
      shell_setup(optarg);
      exit(EXIT_SUCCESS);
      break;
    default:
      help(argv[0]);
      exit(EXIT_SUCCESS);
    }
  }
  if (args->file_path == NULL) {
    fprintf(stderr, "ERROR: you must provide a database file with -f.\n");
    help(argv[0]);
    exit(EXIT_FAILURE);
  }
  if (optind < argc - 1) {
    help(argv[0]);
    exit(EXIT_FAILURE);
  }
  if (optind != argc) {
    args->key = argv[argc - 1];
  }
  if (*(args->key) == 0 && args->mode == MODE_update) {
    fprintf(stderr, "ERROR: nothing to add to the database.\n");
    exit(EXIT_FAILURE);
  }
  return args;
}
