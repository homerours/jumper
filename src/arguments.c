#include <ctype.h>
#include <getopt.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "arguments.h"

static const char VERSION[] = "v1.1";

static const int default_n_results = 50000;
static const char default_dir_database[] = "/.jfolders";
static const char default_files_database[] = "/.jfiles";
static const char default_filters_database[] = "/.jfilters";
static const char directories_env_variable[] = "__JUMPER_FOLDERS";
static const char files_env_variable[] = "__JUMPER_FILES";
static const char filters_env_variable[] = "__JUMPER_FILTERS";

static const char HELP_STRING[] =
    "Usage: %s [MODE] [OPTIONS] ARG\n"
    "MODE has to be one of 'find', 'update', 'clean', 'status', 'shell'.\n\n"
    " -f, --file=FILE_PATH      Path to the database's file. If not supplied\n"
    "                           jumper will use the environment variables\n"
    "                           __JUMPER_FOLDERS or __JUMPER_FILES depending "
    "on --type.\n"
    " -t, --type=TYPE           TYPE has to be 'files' or 'directories'.\n"
    " -F, --filters=FILE_PATH   Path to a file that specifies which pathes to "
    "filter out.\n"
    "                           Jumper uses __JUMPER_FILTERS by default.\n"
    "                           Leave empty to turn filtering off.\n"
    " -h, --help                Display this help and exit.\n"
    " -v, --version             Print version.\n\n"
    "MODE find: look for ARG in the database\n"
    " -n, --n-results=N         Maximum number of results to show.\n"
    " -c, --color               Highlight matches in outputs.\n"
    " -s, --scores              Print the scores of the matches.\n"
    " -b, --beta=BETA           Specify an inverse temperature\n"
    "                           for computing scores (default=1.0).\n"
    " -x, --syntax=syntax       Query syntax (default: extended).\n"
    " -o, --orderless           Orderless queries: tokens can be matched\n"
    "                           in any order (only for extended syntax).\n"
    " -e, --existing            Only print files/directories that exist in the "
    "file system.\n"
    " -I, --case-insensitive    Make the search case-insensitive.\n"
    " -S, --case-sensitive      Make the search case-sensitive.\n"
    " -H, --home-tilde          Substitute $HOME with ~ when printing "
    "results.\n"
    " -r, --relative=PATH       Outputs relative paths to PATH if\n"
    "                           specified (defaults to current directory).\n"
    "MODE update: update the record ARG in the database\n"
    " -w, --weight=WEIGHT       Weight of the visit (default=1.0).\n\n"
    "MODE clean: remove entries that do not exist anymore, or that matches one "
    "of the filters.\n"
    "                           If --type is not specified, cleans both "
    "databases.\n"
    " -D, --dry-run             Create filtered tmp file without replacing the "
    "original database.\n"
    "MODE status: print databases' locations and some statistics.\n"
    "MODE shell: print setup scripts. ARG has to be bash, zsh or fish.\n"
    " -B, --no-bind             Do not bind keys.\n";

static void help(const char *argv0) { printf(HELP_STRING, argv0); }

static void print_version(void) { printf("%s\n", VERSION); }

static struct option longopts[] = {{"file", required_argument, NULL, 'f'},
                                   {"weight", required_argument, NULL, 'w'},
                                   {"scores", no_argument, NULL, 's'},
                                   {"color", no_argument, NULL, 'c'},
                                   {"home-tilde", no_argument, NULL, 'H'},
                                   {"case-insensitive", no_argument, NULL, 'I'},
                                   {"case-sensitive", no_argument, NULL, 'S'},
                                   {"relative", optional_argument, NULL, 'r'},
                                   {"filters", optional_argument, NULL, 'F'},
                                   {"beta", required_argument, NULL, 'b'},
                                   {"n-results", required_argument, NULL, 'n'},
                                   {"syntax", required_argument, NULL, 'x'},
                                   {"orderless", no_argument, NULL, 'o'},
                                   {"existing", no_argument, NULL, 'e'},
                                   {"type", required_argument, NULL, 't'},
                                   {"no-bind", no_argument, NULL, 'B'},
                                   {"dry-run", no_argument, NULL, 'D'},
                                   {NULL, 0, NULL, 0}};

static void args_init(Arguments *args) {
  args->file_path = NULL;
  args->key = NULL;
  args->n_results = default_n_results;
  args->highlight = false;
  args->print_scores = false;
  args->home_tilde = false;
  args->orderless = false;
  args->existing = false;
  args->type = TYPE_undefined;
  args->relative_to = NULL;
  args->filters = NULL;
  args->mode = MODE_search;
  args->syntax = SYNTAX_extended;
  args->case_mode = CASE_MODE_semi_sensitive;
  // frecency(path visited every day) ~ frecency(path visited every two days) +
  // 1 a beta of 1.0 makes this correspond to a difference of +2 in matching
  // score.
  args->beta = 1.0;
  args->weight = 1.0;
  args->no_bind = false;
  args->dry_run = false;
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

static TYPE parse_type(const char *arg) {
  if ((strcmp(arg, "f") == 0) || (strcmp(arg, "files") == 0)) {
    return TYPE_files;
  } else if ((strcmp(arg, "d") == 0) || (strcmp(arg, "directories") == 0)) {
    return TYPE_directories;
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

char *get_home_path() {
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
  return home;
}

char *get_default_database_path(TYPE type) {
  char *path = getenv((type == TYPE_directories) ? directories_env_variable
                                                 : files_env_variable);
  if (path == NULL) {
    char *home = get_home_path();
    path = (char *)malloc((strlen(home) + 20) * sizeof(char));
    strcpy(path, home);
    strcat(path, (type == TYPE_directories) ? default_dir_database
                                            : default_files_database);
  } else {
    path = strdup(path);
  }
  return path;
}

char *get_default_filters_path() {
  char *path = getenv(filters_env_variable);
  if (path == NULL) {
    char *home = get_home_path();
    path = (char *)malloc((strlen(home) + 20) * sizeof(char));
    strcpy(path, home);
    strcat(path, default_filters_database);
  } else {
    path = strdup(path);
  }
  return path;
}

void set_filepath(Arguments *args) {
  if (args->file_path == NULL) {
    if (args->type != TYPE_undefined) {
      // no file path specified, trying to get it through environment variables.
      args->file_path = get_default_database_path(args->type);
    } else {
      fprintf(stderr, "ERROR: no database's file or type specified.\n");
      exit(EXIT_FAILURE);
    }
  }
}
void validate_arguments(Arguments *args) {
  if (args->type == TYPE_undefined && args->existing) {
    fprintf(stderr,
            "ERROR: missing --type when using the --existing (-e) flag.\n");
    exit(EXIT_FAILURE);
  }
  switch (args->mode) {
  case MODE_search:
    if (args->key == NULL) {
      args->key = "";
    }
    set_filepath(args);
    break;
  case MODE_update:
    set_filepath(args);
    if (args->key == NULL) {
      fprintf(stderr, "ERROR: nothing to add to the database.\n");
      exit(EXIT_FAILURE);
    }
    if (*args->key == '\0') {
      fprintf(stderr,
              "ERROR: can not add an empty argument to the database.\n");
      exit(EXIT_FAILURE);
    }
    if (strchr(args->key, '|') != NULL) {
      fprintf(stderr, "ERROR: the argument can not contain '|'.\n");
      exit(EXIT_FAILURE);
    }
    break;
  case MODE_clean:
    // If type is specified, set the file path
    // If type is undefined, we'll clean both databases in parallel
    if (args->type != TYPE_undefined) {
      set_filepath(args);
    }
    break;
  case MODE_status:
    if (args->key == NULL) {
      args->key = "";
    }
    if (args->type != TYPE_undefined) {
      set_filepath(args);
    }
    break;
  case MODE_shell:
    if (args->key == NULL) {
      fprintf(stderr, "ERROR: missing argument for shell mode.\n");
      exit(EXIT_FAILURE);
    }
    break;
  default:
    return;
  }
}
Arguments *parse_arguments(int argc, char **argv) {

  Arguments *args = (Arguments *)malloc(sizeof(Arguments));
  if (!args) {
    fprintf(stderr, "ERROR: Failed to allocate memory for arguments.\n");
    exit(EXIT_FAILURE);
  }
  args_init(args);

  if (argc == 1) {
    help(argv[0]);
    exit(EXIT_SUCCESS);
  }
  if (strcmp(argv[1], "help") == 0 || strcmp(argv[1], "--help") == 0 ||
      strcmp(argv[1], "-h") == 0) {
    help(argv[0]);
    exit(EXIT_SUCCESS);
  }
  if (strcmp(argv[1], "version") == 0 || strcmp(argv[1], "--version") == 0 ||
      strcmp(argv[1], "-v") == 0) {
    print_version();
    exit(EXIT_SUCCESS);
  }
  args->mode = parse_mode(argv[1]);
  if (args->mode == MODE_status) {
    args->n_results = 3;
  }
  args->filters = get_default_filters_path();
  optind++;
  int c = 0;
  while (optind < argc && c != -1) {
    c = getopt_long(argc, argv, "csoeHISt:f:n:w:b:x:r::F::BD", longopts, NULL);
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
      case 'e':
        args->existing = true;
        break;
      case 'c':
        args->highlight = true;
        break;
      case 'H':
        args->home_tilde = true;
        break;
      case 't':
        args->type = parse_type(optarg);
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
        if (optarg == NULL) {
          args->relative_to = getcwd(NULL, 0);
        } else {
          args->relative_to = optarg;
        }
        break;
      case 'F':
        free((void *)args->filters);
        args->filters = optarg;
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
                  "ERROR: The number of results -n has to be non-negative.\n");
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
      case 'B':
        args->no_bind = true;
        break;
      case 'D':
        args->dry_run = true;
        break;
      default:
        help(argv[0]);
        exit(EXIT_SUCCESS);
      }
    } else if (optind == argc - 1) {
      args->key = argv[argc - 1];
    } else {
      fprintf(stderr, "ERROR: unknown argument %s\n", argv[optind]);
      exit(EXIT_FAILURE);
    }
  }
  validate_arguments(args);
  return args;
}
