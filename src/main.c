#include <dirent.h>
#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "constants.h"
#include "operations.h"
#include "parser.h"

int is_job_file(const char *file_name) {
  const char *dot = strrchr(file_name, '.'); // last dot on file_name
  if (dot != NULL && strcmp(dot, ".job") == 0)
    return 1;
  return 0;
}

int main(int argc, char *argv[]) {
  if (kvs_init()) {
    fprintf(stderr, "Failed to initialize KVS\n");
    return 1;
  }

  // debug dos args
  printf("argc: %d\n", argc);
  for (int i = 0; i < argc; i++)
    printf("argv[%d]: %s\n", i, argv[i]);

  // TODO isto é desnecessário
  if (argc < 2) {
    fprintf(stderr, "Usage: %s <job_directory> [MAX_THREADS]\n", argv[0]);
    kvs_terminate();
    return 1;
  }

  char *jobs_dir = argv[1];
  printf("jobs_dir: %s\n", jobs_dir);

  int MAX_THREADS = atoi(argv[2]);
  printf("MAX_THREADS: %d\n", MAX_THREADS);

  DIR *dir = opendir(jobs_dir);
  if (dir == NULL) {
    fprintf(stderr, "Failed to open directory %s\n", jobs_dir);
    kvs_terminate();
    return 1;
  }

  struct dirent *entry;
  while ((entry = readdir(dir)) != NULL) {
    // verify .job extension
    if (!is_job_file(entry->d_name))
      continue;

    printf("After: file_path(entry->d_name): %s\n", entry->d_name);

    // construct full path for the .job file
    // full_path = jobs_dir
    char full_path[PATH_MAX] = ""; // TODO talvez mudar para uma constante do header file
    strncpy(full_path, jobs_dir, PATH_MAX);
    strcat(full_path, "/");
    strcat(full_path, entry->d_name);
    printf("full_path: %s\n", full_path);

    int job_fd = open(full_path, O_RDONLY);
    if (job_fd < 0) {
      fprintf(stderr, "Failed to open job file %s", full_path);
      return 1; // TODO tirar a duvida se o programa acaba ou simplesmente avançamos para outro ficheiro
    }
    printf("Opening file: %s\n", full_path);

    // Create correspondent .out file;
    char out_path[PATH_MAX] = "";
    printf("strlen(full_path): %ld\n", strlen(full_path));
    strncpy(out_path, full_path, strlen(full_path) - 4); // remove ".job"
    strcat(out_path, ".out");
    printf("out_path: %s\n", out_path);

    // se calhar criar uma macro no constants.h para isto
    int open_flags = O_CREAT | O_WRONLY | O_TRUNC;
    // rw-rw-rw
    mode_t file_perms = S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH;


    int out_fd = open(out_path, open_flags, file_perms);
    if (out_fd == -1) {
      fprintf(stderr, "Failed to open .out file\n");
      return 1;
    }

    puts("NOT IMPLEMENTED YET");
  }

  while (1) {
    char keys[MAX_WRITE_SIZE][MAX_STRING_SIZE]   = { 0 };
    char values[MAX_WRITE_SIZE][MAX_STRING_SIZE] = { 0 };
    unsigned int delay;
    size_t num_pairs;

    printf("> ");
    fflush(stdout);

    switch (get_next(STDIN_FILENO)) {
    case CMD_WRITE:
      num_pairs = parse_write(STDIN_FILENO, keys, values, MAX_WRITE_SIZE, MAX_STRING_SIZE);
      if (num_pairs == 0) {
        fprintf(stderr, "Invalid command. See HELP for usage\n");
        continue;
      }

      if (kvs_write(num_pairs, keys, values)) {
        fprintf(stderr, "Failed to write pair\n");
      }

      break;

    case CMD_READ:
      num_pairs = parse_read_delete(STDIN_FILENO, keys, MAX_WRITE_SIZE, MAX_STRING_SIZE);

      if (num_pairs == 0) {
        fprintf(stderr, "Invalid command. See HELP for usage\n");
        continue;
      }

      if (kvs_read(num_pairs, keys)) {
        fprintf(stderr, "Failed to read pair\n");
      }
      break;

    case CMD_DELETE:
      num_pairs = parse_read_delete(STDIN_FILENO, keys, MAX_WRITE_SIZE, MAX_STRING_SIZE);

      if (num_pairs == 0) {
        fprintf(stderr, "Invalid command. See HELP for usage\n");
        continue;
      }

      if (kvs_delete(num_pairs, keys)) {
        fprintf(stderr, "Failed to delete pair\n");
      }
      break;

    case CMD_SHOW: kvs_show(); break;

    case CMD_WAIT:
      if (parse_wait(STDIN_FILENO, &delay, NULL) == -1) {
        fprintf(stderr, "Invalid command. See HELP for usage\n");
        continue;
      }

      if (delay > 0) {
        printf("Waiting...\n");
        kvs_wait(delay);
      }
      break;

    case CMD_BACKUP:

      if (kvs_backup()) {
        fprintf(stderr, "Failed to perform backup.\n");
      }
      break;

    case CMD_INVALID: fprintf(stderr, "Invalid command. See HELP for usage\n"); break;

    case CMD_HELP:
      printf("Available commands:\n"
             "  WRITE [(key,value),(key2,value2),...]\n"
             "  READ [key,key2,...]\n"
             "  DELETE [key,key2,...]\n"
             "  SHOW\n"
             "  WAIT <delay_ms>\n"
             "  BACKUP\n" // Not implemented
             "  HELP\n");

      break;

    case CMD_EMPTY: break;

    case EOC: kvs_terminate(); return 0;
    }
  }
}
