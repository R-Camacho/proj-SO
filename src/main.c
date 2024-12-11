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
#include "thread_manager.h"

size_t MAX_BACKUPS;

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
  if (argc < 4) {                                                                        // menor que 3 ou 2?
    fprintf(stderr, "Usage: %s <job_directory> <MAX_BACKUPS> <MAX_THREADS>\n", argv[0]); // ajustar estes parenteses
    kvs_terminate();
    return 1;
  }

  char *jobs_dir = argv[1];
  printf("jobs_dir: %s\n", jobs_dir);

  MAX_BACKUPS = strtoul(argv[2], NULL, 10);
  printf("MAX_BACKUPS: %lu\n", MAX_BACKUPS);

  size_t MAX_THREADS = strtoul(argv[3], NULL, 10);
  printf("MAX_THREADS: %lu\n", MAX_THREADS);

  DIR *dir = opendir(jobs_dir);
  if (!dir) {
    fprintf(stderr, "Failed to open directory %s\n", jobs_dir);
    kvs_terminate();
    return 1;
  }

  thread_manager_init(MAX_THREADS);

  struct dirent *entry;
  while ((entry = readdir(dir)) != NULL) {
    // verify .job extension
    if (!is_job_file(entry->d_name))
      continue;

    // construct full path for the .job file
    // job_path = jobs_dir/entry->d_name
    char job_path[PATH_MAX] = ""; // TODO talvez mudar para uma constante do header file
    strncpy(job_path, jobs_dir, PATH_MAX);
    strcat(job_path, "/");
    strcat(job_path, entry->d_name);

    thread_manager_add_job(job_path);
  }
  closedir(dir);

  thread_manager_destroy();

  return kvs_terminate();
}
