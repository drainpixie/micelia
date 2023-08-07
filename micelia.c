#include <ctype.h>
#include <dirent.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#define MAX_INT_LEN 8

#define STREQ(a, b) (strcmp((a), (b)) == 0)
#define UNRECOVERABLE(message)                                                 \
  {                                                                            \
    fprintf(stderr, message);                                                  \
    exit(EXIT_FAILURE);                                                        \
  }

static int max_file_path_len = 0;

typedef struct t_node {
  char *file_path;
  struct t_node *next;
} t_node;

typedef struct t_files_stats {
  int code_lines;
  int blank_lines;
  int comment_lines;
} t_files_stats;

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
  stats->comment_lines = 0;

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

int get_max_str_len() {
  return max_file_path_len + 4; // Adding 4 for padding
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

static char *base_path;

static t_files_stats *stats;
static t_node *files;

static const char *inline_comments[] = {"//", ";;", "#"};
static const char *ignore_folders[] = {".git", ".vscode", "node_modules",
                                       ".idea"};

static const char *ignore_list[1024];
static int ignore_count = 0;

int readdir_recursive(const char *path) {
  DIR *folder;
  struct dirent *entry;
  char full_path[PATH_MAX];

  folder = opendir(path);
  if (!folder)
    return EXIT_FAILURE;

  while ((entry = readdir(folder)) != NULL) {
    if (STREQ(entry->d_name, ".") || STREQ(entry->d_name, ".."))
      continue;

    bool should_ignore = false;
    for (size_t i = 0; i < sizeof(ignore_folders) / sizeof(ignore_folders[0]);
         i++) {
      if (STREQ(entry->d_name, ignore_folders[i])) {
        should_ignore = true;
        break;
      }
    }

    // Check against user-added ignore list
    for (int i = 0; i < ignore_count; i++) {
      if (STREQ(entry->d_name, ignore_list[i])) {
        should_ignore = true;
        break;
      }
    }

    if (should_ignore)
      continue;

    snprintf(full_path, sizeof(full_path), "%s/%s", path, entry->d_name);

    if (entry->d_type == DT_REG) {
      char *relative_path = (char *)malloc(strlen(full_path) + 1);
      sprintf(relative_path, "%s", full_path + strlen(base_path) + 1);

      insert_node(&files, create_node(relative_path));
      free(relative_path);
    } else if (entry->d_type == DT_DIR)
      readdir_recursive(full_path);
  }

  closedir(folder);
  return EXIT_SUCCESS;
}

// TODO: implement some sort of parser for comments,
//       or look at how other people do it.
bool is_comment(const char *line) {
  for (size_t i = 0; i < sizeof(inline_comments) / sizeof(inline_comments[0]);
       i++) {
    const char *comment_symbol = inline_comments[i];
    size_t comment_length = strlen(comment_symbol);

    if (strncmp(line, comment_symbol, comment_length) == 0)
      return true;
  }

  return false;
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
  int comment_lines = 0;

  while ((read = getline(&line, &len, file)) != -1) {
    if (is_comment(line))
      comment_lines++;
    else if (!is_empty(line))
      code_lines++;
    else
      blank_lines++;
  }
  free(line);
  fclose(file);
  printf("%-*.*s | %*d | %*d | %*d\n", get_max_str_len(), get_max_str_len(),
         path, MAX_INT_LEN, code_lines, MAX_INT_LEN, comment_lines, MAX_INT_LEN,
         blank_lines);

  stats->code_lines += code_lines;
  stats->blank_lines += blank_lines;
  stats->comment_lines += comment_lines;
}

int main(int argc, char *argv[]) {
  int opt;
  while ((opt = getopt(argc, argv, "i:")) != -1) {
    switch (opt) {
    case 'i':
      if (ignore_count >= sizeof(ignore_list) / sizeof(ignore_list[0])) {
        UNRECOVERABLE("Too many ignore options.\n");
      }
      ignore_list[ignore_count++] = optarg;
      break;
    default:
      UNRECOVERABLE("Invalid option.\n");
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

  // TODO: find a way to DRY this, whilst being generic.
  printf("%-*.*s | %*.*s | %*.*s | %*.*s\n", get_max_str_len(),
         get_max_str_len(), "file", MAX_INT_LEN, MAX_INT_LEN, "code",
         MAX_INT_LEN, MAX_INT_LEN, "comment", MAX_INT_LEN, MAX_INT_LEN,
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

  printf("%-*.*s | %*d | %*d | %*d\n", get_max_str_len(), get_max_str_len(),
         "total", MAX_INT_LEN, stats->code_lines, MAX_INT_LEN,
         stats->comment_lines, MAX_INT_LEN, stats->blank_lines);

  free_list(files);
  free_stats(stats);

  return EXIT_SUCCESS;
}
