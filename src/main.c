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
#include "thread_manager.h"

size_t MAX_BACKUPS;

int is_job_file(const char *file_name) {
  const char *dot = strrchr(file_name, '.'); // last dot on file_name
  if (dot != NULL && strcmp(dot, ".job") == 0)
    return 1;
  return 0;
}

/**
 * @file main.c
 * @brief Main entry point for the job processing application.
 *
 * This file contains the main function which initializes the key-value store (KVS),
 * processes command-line arguments, and manages job processing using threads.
 *
 * Usage: ./kvs <job_directory> <MAX_BACKUPS> <MAX_THREADS>
 *
 * @param argc Number of command-line arguments.
 * @param argv Array of command-line arguments.
 *             argv[0] - Program name
 *             argv[1] - Directory containing job files
 *             argv[2] - Maximum number of backups
 *             argv[3] - Maximum number of threads
 *
 * @return 0 on success, 1 on failure.
 *
 * The program performs the following steps:
 * 1. Initializes the key-value store (KVS).
 * 2. Validates the number of command-line arguments.
 * 3. Parses the job directory, maximum backups, and maximum threads from the arguments.
 * 4. Opens the job directory and processes each file with a .job extension.
 * 5. Adds each job file to the thread manager for processing.
 * 6. Cleans up resources and terminates the KVS.
 */
int main(int argc, char *argv[]) {
  // Initializes the key-value store (KVS)
  if (kvs_init()) {
    fprintf(stderr, "Failed to initialize KVS\n");
    return 1;
  }

  if (argc < 4) {
    fprintf(stderr, "Usage: %s <job_directory> <MAX_BACKUPS> <MAX_THREADS>\n", argv[0]);
    kvs_terminate();
    return 1;
  }

  char *jobs_dir = argv[1]; // Directory containing job files

  MAX_BACKUPS = strtoul(argv[2], NULL, 10); // Maximum number of backups

  size_t MAX_THREADS = strtoul(argv[3], NULL, 10); // Maximum number of threads

  DIR *dir = opendir(jobs_dir); // Open the job directory
  if (!dir) {
    fprintf(stderr, "Failed to open directory %s\n", jobs_dir);
    kvs_terminate();
    return 1;
  }

  if (thread_manager_init(MAX_THREADS) != 0) { // Initializes all the threads
    fprintf(stderr, "Failed to initialize thread manager\n");
    kvs_terminate();
    return 1;
  }

  // Process each file with a .job extension in the job directory
  struct dirent *entry;
  while ((entry = readdir(dir)) != NULL) {

    // verify .job extension
    if (!is_job_file(entry->d_name))
      continue;

    // construct full path for the .job file (job_path = jobs_dir/entry->d_name)
    char job_path[PATH_MAX] = "";
    strncpy(job_path, jobs_dir, PATH_MAX);
    strcat(job_path, "/");
    strcat(job_path, entry->d_name);

    // Add the job file to a thread for processing
    thread_manager_add_job(job_path);
  }

  closedir(dir);            // Close the job directory
  thread_manager_destroy(); // Destroys all the threads

  return kvs_terminate();
}
