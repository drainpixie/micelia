#include <ctype.h>
#include <dirent.h>
#include <getopt.h>
#include <linux/limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define STREQ(a, b) (strcmp((a), (b)) == 0)
#define UNRECOVERABLE(message)                                                 \
  {                                                                            \
    fprintf(stderr, message);                                                  \
    exit(EXIT_FAILURE);                                                        \
  }

static char *base_path;

static int max_file_path_len = 0;
// making max_int_len dynamic isn't impossible but,
// realistically a waste of time and the best solution
// i can think of is fairly ugly :/
static int max_int_len = 9;

typedef struct t_node {
  char *file_path;
  struct t_node *next;
} t_node;

typedef struct t_files_stats {
  int code_lines;
  int blank_lines;
} t_files_stats;

static t_files_stats *stats;

static t_node *ignore_list;
static t_node *files;

t_node *create_node(const char *file_path) {
  t_node *new_node = (t_node *)malloc(sizeof(t_node));

  new_node->file_path = strdup(file_path);
  new_node->next = NULL;

  return new_node;
}

t_files_stats *create_stats() {
  t_files_stats *stats = (t_files_stats *)malloc(sizeof(t_files_stats));

  stats->code_lines = 0;
  stats->blank_lines = 0;

  return stats;
}

void insert_node(t_node **head, t_node *new_node) {
  int len = strlen(new_node->file_path);
  if (len > max_file_path_len) {
    max_file_path_len = len;
  }

  if (*head == NULL)
    *head = new_node;
  else {
    t_node *current = *head;
    while (current->next != NULL)
      current = current->next;

    current->next = new_node;
  }
}

bool includes_node(t_node *head, char *target) {
  t_node *current = head;

  while (current != NULL) {
    if (STREQ(current->file_path, target))
      return true;

    current = current->next;
  }

  return false;
}

int get_max_str_len() {
  return max_file_path_len + 4; // +4 for padding
}

void free_list(t_node *head) {
  while (head != NULL) {
    t_node *temp = head;
    head = head->next;
    free(temp->file_path);
    free(temp);
  }
}

void free_stats(t_files_stats *stats) { free(stats); }

int readdir_recursive(const char *path) {
  DIR *folder;
  struct dirent *entry;
  char full_path[PATH_MAX];

  folder = opendir(path);
  if (!folder)
    return EXIT_FAILURE;

  while ((entry = readdir(folder)) != NULL) {
    if (includes_node(ignore_list, entry->d_name))
      continue;

    snprintf(full_path, sizeof(full_path), "%s/%s", path, entry->d_name);

    if (entry->d_type == DT_REG) {
      char *relative_path = (char *)malloc(strlen(full_path) + 1);
      sprintf(relative_path, "%s", full_path + strlen(base_path) + 1);

      insert_node(&files, create_node(relative_path));
      free(relative_path);
    } else if (entry->d_type == DT_DIR) {
      readdir_recursive(full_path);
    }
  }

  closedir(folder);
  return EXIT_SUCCESS;
}

bool is_empty(const char *line) {
  while (*line) {
    if (!isspace(*line))
      return false;
    line++;
  }

  return true;
}

void count_file(const char *path) {
  char *line = NULL;

  size_t len = 0;
  ssize_t read;

  FILE *file = fopen(path, "r");
  if (!file)
    UNRECOVERABLE("not a file.\n");

  int code_lines = 0;
  int blank_lines = 0;

  while ((read = getline(&line, &len, file)) != -1) {
    if (!is_empty(line))
      code_lines++;
    else
      blank_lines++;
  }

  free(line);
  fclose(file);

  printf("%-*.*s | %*d | %*d\n", get_max_str_len(), get_max_str_len(),
         path, max_int_len, code_lines, max_int_len,
         blank_lines);

  stats->code_lines += code_lines;
  stats->blank_lines += blank_lines;
}

void display_help() {
  printf("usage: micelia [OPTIONS] PATH\n");
  printf("options:\n");
  printf("  -i, --ignore IGNORE_PATH   ignore a specific path\n");
  printf("  -h, --help                 display this help message\n");
  exit(EXIT_SUCCESS);
}

int main(int argc, char *argv[]) {
  insert_node(&ignore_list, create_node("."));
  insert_node(&ignore_list, create_node(".."));
  insert_node(&ignore_list, create_node(".git"));
  insert_node(&ignore_list, create_node(".idea"));
  insert_node(&ignore_list, create_node(".vscode"));
  insert_node(&ignore_list, create_node("node_modules"));

  int opt;
  int option_index = 0;

  static struct option long_options[] = {{"ignore", required_argument, 0, 'i'},
                                         {"help", no_argument, 0, 'h'},
                                         {0, 0, 0, 0}};

  while ((opt = getopt_long(argc, argv, "i:h", long_options, &option_index)) !=
         -1) {
    switch (opt) {
    case 'i':
      insert_node(&ignore_list, create_node(optarg));
      break;
    case 'h':
      display_help();
      break;
    default:
      UNRECOVERABLE("invalid option.\n");
    }
  }

  if (optind >= argc)
    UNRECOVERABLE("no path provided.\n");

  base_path = argv[optind];
  size_t path_len = strlen(base_path);

  if (path_len > 0 && base_path[path_len - 1] == '/')
    base_path[path_len - 1] = '\0';

  int res = readdir_recursive(base_path);
  stats = create_stats();

  // it's kinda ugly and repetitive but, we only do this a few
  // times in the code-base, and i couldn't find a good way
  // to make it generic enough.
  printf("%-*.*s | %*.*s | %*.*s\n", get_max_str_len(), get_max_str_len(),
         "file", max_int_len, max_int_len, "code", max_int_len, max_int_len,
         "blank");

  if (res == EXIT_SUCCESS) {
    char *file_path;

    t_node *current = files;
    while (current != NULL) {
      file_path =
          (char *)malloc(strlen(base_path) + strlen(current->file_path) + 2);
      sprintf(file_path, "%s/%s", base_path, current->file_path);

      count_file(file_path);
      current = current->next;

      free(file_path);
    }
  } else
    count_file(base_path);

  printf("%-*.*s | %*d | %*d\n", get_max_str_len(), get_max_str_len(),
         "total", max_int_len, stats->code_lines, max_int_len,
         stats->blank_lines);

  free_list(files);
  free_list(ignore_list);
  free_stats(stats);

  return EXIT_SUCCESS;
}
