#include <dirent.h>
#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "constants.h"
#include "operations.h"
#include "parser.h"

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

  DIR *dir = opendir(jobs_dir);
  if (!dir) {
    fprintf(stderr, "Failed to open directory %s\n", jobs_dir);
    kvs_terminate();
    return 1;
  }

  struct dirent *entry;
  while ((entry = readdir(dir))) {
    // verify .job extension
    if (strstr(entry->d_name, ".job") != NULL)
      continue;

    // construct full path for the .job file
    char job_path[PATH_MAX];
    snprintf(job_path, sizeof(job_path), "%s/%s", jobs_dir, entry->d_name);

    int job_fd = open(job_path, O_RDONLY);
    if (job_fd == -1) {
      fprintf(stderr, "Failed to open job file %s", job_path);
      // return 1; TODO tirar a duvida se o programa acaba ou simplesmente avançamos para outro ficheiro
    }
    printf("Opening file: %s\n", job_path);


    char out_name[NAME_MAX];
    strncpy(out_name, entry->d_name, NAME_MAX);
    char *dot = strchr(out_name, '.');
    if (dot != NULL)
      strcpy(dot, ".out");
    else
      strcat(out_name, ".out");

    printf("out_name: %s\n", out_name);

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
