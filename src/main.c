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
#include "reader.h"

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

  // TODO isto é desnecessário
  if (argc < 3) {                                                          // menor que 3 ou 2?
    fprintf(stderr, "Usage: %s <job_directory> [MAX_THREADS]\n", argv[0]); // ajustar estes parenteses
    kvs_terminate();
    return 1;
  }

  char *jobs_dir = argv[1];
  printf("jobs_dir: %s\n", jobs_dir);

  int MAX_THREADS = atoi(argv[2]);
  printf("MAX_THREADS: %d\n", MAX_THREADS);

  DIR *dir = opendir(jobs_dir);
  if (!dir) {
    fprintf(stderr, "Failed to open directory %s\n", jobs_dir);
    kvs_terminate();
    return 1;
  }

  struct dirent *entry;
  while ((entry = readdir(dir)) != NULL) {
    // verify .job extension
    if (!is_job_file(entry->d_name))
      continue;

    // construct full path for the .job file
    // job_path = jobs_dir
    char job_path[PATH_MAX] = ""; // TODO talvez mudar para uma constante do header file
    strncpy(job_path, jobs_dir, PATH_MAX);
    strcat(job_path, "/");
    strcat(job_path, entry->d_name);

    int job_fd = open(job_path, O_RDONLY);
    if (job_fd < 0) {
      fprintf(stderr, "Failed to open job file %s", job_path);
      kvs_terminate();
      return 1; // TODO tirar a duvida se o programa acaba ou simplesmente avançamos para outro ficheiro
    }
    printf("Opening file: %s\n", job_path);

    // Create correspondent .out file;
    char out_path[PATH_MAX] = "";
    strncpy(out_path, job_path, strlen(job_path) - 4); // remove ".job"
    strcat(out_path, ".out");

    // se calhar criar uma macro no constants.h para estas duas
    int open_flags = O_CREAT | O_WRONLY | O_TRUNC;
    // rw-rw-rw (or 0666)
    mode_t file_perms = S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH;

    int out_fd = open(out_path, open_flags, file_perms);
    if (out_fd < 0) {
      fprintf(stderr, "Failed to open .out file:%s \n", out_path);
      kvs_terminate(); // TODO terminar ou avancar para outro ?
      return 1;
    }

    read_file(job_fd, out_fd);

    // TODO fechar os ficheiros aqui
    if (close(job_fd) < 0) {
      fprintf(stderr, "Failed to close .job file: %s\n", job_path);
      kvs_terminate();
    }

    if (close(out_fd) < 0) {
      fprintf(stderr, "Failed to close .out file: %s\n", out_path);
      kvs_terminate();
    }
  }
  free(dir);

  return kvs_terminate();
}
