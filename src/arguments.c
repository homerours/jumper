#include <ctype.h>
#include <getopt.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "arguments.h"
#include "shell.h"

static const int default_n_results = 50000;
static const size_t max_cwd_len = 256;
static const char default_dir_database[] = "/.jfolders";
static const char default_files_database[] = "/.jfiles";
static const char directories_env_variable[] = "__JUMPER_FOLDERS";
static const char files_env_variable[] = "__JUMPER_FILES";

static const char HELP_STRING[] =
    "Usage: %s [MODE] [OPTIONS] ARG\n"
    "MODE has to be one of 'find', 'update', 'clean', 'status', 'shell'.\n\n"
    " -f, --file=FILE_PATH      Path to the database file. If not supplied\n"
    "                           jumper will use the environment variables\n"
    "                           __JUMPER_FOLDERS or __JUMPER_FILES depending "
    "on --type.\n"
    " -t, --type=TYPE           TYPE has to be 'files' or 'directories' "
    "(default).\n"
    " -h, --help                Display this help and exit.\n\n"
    "MODE find: query ARG in the database\n"
    " -n, --n-results=N         Maximum number of results to show.\n"
    " -c, --color               Highlight matches in outputs.\n"
    " -s, --scores              Print the scores of the matches.\n"
    " -b, --beta=BETA           Specify an inverse temperature\n"
    "                           when computing the score (default=1.0).\n"
    " -x, --syntax=syntax       Query syntax (default: extended).\n"
    " -o, --orderless           Orderless queries: token can be matched\n"
    "                           in any order (only for extended syntax).\n"
    " -I, --case-insensitive    Make the search case-insensitive.\n"
    " -S, --case-sensitive      Make the search case-senstitive.\n"
    " -H, --home-tilde          Substitute $HOME with ~ when printing "
    "results.\n"
    " -r, --relative=PATH       Outputs relative paths to PATH if\n"
    "                           specified (defaults to current directory).\n"
    "MODE update: update the record ARG in the database\n"
    " -w, --weight=WEIGHT       Weight of the visit (default=1.0).\n\n"
    "MODE clean: remove entries that do not exist anymore.\n"
    "MODE status: print databases' locations and some statistics.\n"
    "MODE shell: print setup scripts: ARG has to be bash, zsh or fish.\n";

static void help(const char *argv0) { printf(HELP_STRING, argv0); }

static struct option longopts[] = {
    {"file", required_argument, NULL, 'f'},
    {"add", no_argument, NULL, 'a'},
    {"weight", required_argument, NULL, 'w'},
    {"scores", no_argument, NULL, 's'},
    {"color", no_argument, NULL, 'c'},
    {"home-tilde", no_argument, NULL, 'H'},
    {"case-insensitive", no_argument, NULL, 'I'},
    {"case-sensitive", no_argument, NULL, 'S'},
    {"relative", optional_argument, NULL, 'r'},
    {"beta", required_argument, NULL, 'b'},
    {"n-results", required_argument, NULL, 'n'},
    {"syntax", required_argument, NULL, 'x'},
    {"orderless", no_argument, NULL, 'o'},
    {"type", required_argument, NULL, 't'},
    {"help", no_argument, NULL, 'h'},
    {NULL, 0, NULL, 0}};

static void args_init(Arguments *args) {
  args->file_path = NULL;
  args->key = "";
  args->n_results = default_n_results;
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

static MODE parse_mode(const char *mode) {
  if (strcmp(mode, "find") == 0) {
    return MODE_search;
  } else if (strcmp(mode, "update") == 0) {
    return MODE_update;
  } else if (strcmp(mode, "clean") == 0) {
    return MODE_clean;
  } else if (strcmp(mode, "shell") == 0) {
    return MODE_shell;
  } else if (strcmp(mode, "status") == 0) {
    return MODE_status;
  }
  fprintf(stderr, "ERROR: Invalid argument: %s\n", mode);
  fprintf(stderr, "Accepted arguments: find, update, clean, status, shell.\n");
  exit(EXIT_FAILURE);
}

static bool parse_is_dir(const char *arg) {
  if (strcmp(arg, "files") == 0) {
    return false;
  } else if (strcmp(arg, "directories") == 0) {
    return true;
  }
  fprintf(stderr, "ERROR: Invalid argument for -t (--type): %s\n", arg);
  fprintf(stderr, "Accepted arguments: files, directories.\n");
  exit(EXIT_FAILURE);
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

char *get_default_database_path(bool is_dir) {
  char *path = getenv(is_dir ? directories_env_variable : files_env_variable);
  if (path == NULL) {
    char *home = getenv("HOME");
    if (home == NULL) {
      fprintf(stderr, "ERROR: Could not access $HOME environment variable.\n");
      fprintf(stderr,
              "Please set the environment variables %s and "
              "%s to paths to jumper's files and folder's "
              "databases.\n",
              directories_env_variable, files_env_variable);
      exit(EXIT_FAILURE);
    }
    path = (char *)malloc((strlen(home) + 20) * sizeof(char));
    strcpy(path, home);
    strcat(path, is_dir ? default_dir_database : default_files_database);
  }
  return path;
}

Arguments *parse_arguments(int argc, char **argv) {

  Arguments *args = (Arguments *)malloc(sizeof(Arguments));
  args_init(args);

  if (argc == 1) {
    help(argv[0]);
    exit(EXIT_SUCCESS);
  }
  args->mode = parse_mode(argv[1]);
  if (args->mode == MODE_status) {
    args->n_results = 3;
  }
  if (args->mode == MODE_shell) {
    shell_setup(argv[2]);
    exit(EXIT_SUCCESS);
  }
  optind++;
  int c = 0;
  while (optind < argc && c != -1) {
    c = getopt_long(argc, argv, "cshoHISt:f:n:w:b:x:r::", longopts, NULL);
    if (c != -1) {
      switch (c) {
      case 'f':
        args->file_path = optarg;
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
      case 't':
        args->is_dir = parse_is_dir(optarg);
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
        if (args->n_results < 0) {
          fprintf(stderr,
                  "ERROR: The number of results -n has to be non-positive.\n");
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
      default:
        help(argv[0]);
        exit(EXIT_SUCCESS);
      }
    } else {
      args->key = argv[argc - 1];
    }
  }
  if (args->file_path == NULL) {
    // no file path specified, trying to get it through environment variables.
    args->file_path = get_default_database_path(args->is_dir);
  }

  if (*(args->key) == 0 && args->mode == MODE_update) {
    fprintf(stderr, "ERROR: nothing to add to the database.\n");
    exit(EXIT_FAILURE);
  }
  return args;
}
