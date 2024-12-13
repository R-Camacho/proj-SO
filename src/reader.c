#include "reader.h"

pthread_mutex_t backup_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t backup_cond   = PTHREAD_COND_INITIALIZER;
size_t active_backups        = 0;

/**
 * @brief Reads commands from an input file descriptor and executes them.
 *
 * This function continuously reads commands from the input file descriptor `in_fd`,
 * processes them, and performs the corresponding actions. The results or outputs
 * are written to the output file descriptor `out_fd`. The function supports the
 * following commands:
 *
 * - WRITE [(key,value)(key2,value2),...]: Writes key-value pairs to the key-value store.
 * - READ [key,key2,...]: Reads values for the specified keys from the key-value store.
 * - DELETE [key,key2,...]: Deletes the specified keys from the key-value store.
 * - SHOW: Displays all key-value pairs in the key-value store.
 * - WAIT <delay_ms>: Waits for the specified delay in milliseconds.
 * - BACKUP: Creates a backup of the key-value store.
 * - HELP: Displays the available commands and their usage.
 *
 * @param in_fd The input file descriptor to read commands from.
 * @param out_fd The output file descriptor to write results or outputs to.
 * @param job_path The path to the job file for backup operations.
 */
void read_file(int in_fd, int out_fd, const char *job_path) {
  size_t backup_count = 0;

  // help command string
  const char help[] = "Available commands:\n"
                      "  WRITE [(key,value)(key2,value2),...]\n"
                      "  READ [key,key2,...]\n"
                      "  DELETE [key,key2,...]\n"
                      "  SHOW\n"
                      "  WAIT <delay_ms>\n"
                      "  BACKUP\n"
                      "  HELP\n";

  while (1) {
    char keys[MAX_WRITE_SIZE][MAX_STRING_SIZE]   = { 0 };
    char values[MAX_WRITE_SIZE][MAX_STRING_SIZE] = { 0 };
    unsigned int delay; // delay in milliseconds
    size_t num_pairs;

    switch (get_next(in_fd)) {
    case CMD_WRITE:
      num_pairs = parse_write(in_fd, keys, values, MAX_WRITE_SIZE, MAX_STRING_SIZE);
      if (num_pairs == 0) {
        fprintf(stderr, "Invalid command. See HELP for usage\n");
        continue;
      }

      if (kvs_write(num_pairs, keys, values)) {
        fprintf(stderr, "Failed to write pair\n");
      }

      break;

    case CMD_READ:
      num_pairs = parse_read_delete(in_fd, keys, MAX_WRITE_SIZE, MAX_STRING_SIZE);

      if (num_pairs == 0) {
        fprintf(stderr, "Invalid command. See HELP for usage\n");
        continue;
      }

      if (kvs_read(num_pairs, keys, out_fd)) {
        fprintf(stderr, "Failed to read pair\n");
      }
      break;

    case CMD_DELETE:
      num_pairs = parse_read_delete(in_fd, keys, MAX_WRITE_SIZE, MAX_STRING_SIZE);

      if (num_pairs == 0) {
        fprintf(stderr, "Invalid command. See HELP for usage\n");
        continue;
      }

      if (kvs_delete(num_pairs, keys, out_fd)) {
        fprintf(stderr, "Failed to delete pair\n");
      }
      break;

    case CMD_SHOW:
      kvs_show(out_fd);
      break;

    case CMD_WAIT:
      if (parse_wait(in_fd, &delay, NULL) == -1) {
        fprintf(stderr, "Invalid command. See HELP for usage\n");
        continue;
      }

      if (delay > 0) {
        const char wait_message[] = "Waiting...\n";
        if (write(out_fd, wait_message, strlen(wait_message)) != (ssize_t)strlen(wait_message)) {
          fprintf(stderr, "Failed to write wait to .out file\n");
          break;
        }
        kvs_wait(delay);
      }
      break;

    case CMD_BACKUP:
      backup_count++;

      // lock de backup_mutex enquanto active_backups >= MAX_BACKUPS
      pthread_mutex_lock(&backup_mutex);
      while (active_backups >= MAX_BACKUPS) {
        pthread_cond_wait(&backup_cond, &backup_mutex);
      }
      active_backups++; // quando receber o sinal, incrementa o numero de backups ativos
      pthread_mutex_unlock(&backup_mutex);

      pid_t pid = fork();
      if (pid < 0) { // error handling
        fprintf(stderr, "Failed to fork\n");
        pthread_mutex_lock(&backup_mutex);
        active_backups--;
        pthread_cond_signal(&backup_cond);
        pthread_mutex_unlock(&backup_mutex);
        break;
      } else if (pid == 0) { // Child process
        if (kvs_backup(job_path, backup_count)) {
          fprintf(stderr, "Failed to perform backup %zu\n", backup_count);
          _exit(1);
        }
        _exit(0);
      }
      // Parent
      pthread_mutex_lock(&backup_mutex);
      active_backups--;
      pthread_cond_signal(&backup_cond);
      pthread_mutex_unlock(&backup_mutex);
      break;

    case CMD_INVALID:
      fprintf(stderr, "Invalid command. See HELP for usage\n");
      break;

    case CMD_HELP:
      if (write(out_fd, help, strlen(help)) != (ssize_t)strlen(help)) {
        fprintf(stderr, "Failed to write help to .out file\n");
        break;
      }

      break;

    case CMD_EMPTY:
      break;

    case EOC:
      return;
    }
  }
}
