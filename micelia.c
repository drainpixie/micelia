#include <dirent.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#define MAX_STR_LEN 24
#define MAX_INT_LEN 8

#define STREQ(a, b) (strcmp((a), (b)) == 0)
#define UNRECOVERABLE(message)                                                 \
  {                                                                            \
    fprintf(stderr, message);                                                  \
    exit(EXIT_FAILURE);                                                        \
  }

typedef struct t_node {
  char *file_path;
  struct t_node *next;
} t_node;

typedef struct t_files_stats {
  int code_lines;
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
  stats->comment_lines = 0;

  return stats;
}

void insert_node(t_node **head, t_node *new_node) {
  if (*head == NULL)
    *head = new_node;
  else {
    t_node *current = *head;
    while (current->next != NULL)
      current = current->next;

    current->next = new_node;
  }
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
// TODO: add a CLI option to ignore folders.
static const char *ignore_folders[] = {".git", ".vscode", "node_modules"};

void readdir_recursive(const char *path) {
  DIR *folder;
  struct dirent *entry;
  char full_path[PATH_MAX];

  folder = opendir(path);
  if (!folder)
    UNRECOVERABLE("not a folder.\n");

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

void count_file(const char *path) {
  char *line = NULL;

  size_t len = 0;
  ssize_t read;

  FILE *file = fopen(path, "r");
  if (!file)
    UNRECOVERABLE("not a file.\n");

  int comment_lines = 0;
  int code_lines = 0;

  while ((read = getline(&line, &len, file)) != -1) {
    // TODO: ignore blank linesI
    if (is_comment(line))
      comment_lines++;
    else
      code_lines++;
  }

  free(line);
  fclose(file);

  printf("%-*.*s | %*d | %*d\n", MAX_STR_LEN, MAX_STR_LEN, path, MAX_INT_LEN,
         code_lines, MAX_INT_LEN, comment_lines);

  stats->code_lines += code_lines;
  stats->comment_lines += comment_lines;
}

int main(int argc, char *argv[]) {
  if (argc != 2)
    UNRECOVERABLE("no path provided.\n");

  // TODO: count even if it isn't a folder.
  base_path = argv[1];
  size_t path_len = strlen(base_path);

  if (path_len > 0 && base_path[path_len - 1] == '/')
    base_path[path_len - 1] = '\0';

	// TODO: mime-type based ignore or even just a simple
	//			 extension list based one
  readdir_recursive(base_path);

  stats = create_stats();

  // TODO: find a way to DRY this, whilst being generic.
  printf("%-*.*s | %*.*s | %*.*s\n", MAX_STR_LEN, MAX_STR_LEN, "file",
         MAX_INT_LEN, MAX_INT_LEN, "code", MAX_INT_LEN, MAX_INT_LEN,
         "comments");

  t_node *current = files;
  char *file_path;
  while (current != NULL) {
    file_path =
        (char *)malloc(strlen(base_path) + strlen(current->file_path) + 2);
    sprintf(file_path, "%s/%s", base_path, current->file_path);

    count_file(file_path);
    current = current->next;

		free(file_path);
  }

  printf("%-*.*s | %*d | %*d\n", MAX_STR_LEN, MAX_STR_LEN, "total", MAX_INT_LEN,
         stats->code_lines, MAX_INT_LEN, stats->comment_lines);


  free_list(files);
  free_stats(stats);

  return EXIT_SUCCESS;
}
