#include "reader.h"

extern sem_t sem;

void read_file(int in_fd, int out_fd, const char *job_path) {
  size_t backup = 0;

  while (1) {
    char keys[MAX_WRITE_SIZE][MAX_STRING_SIZE]   = { 0 };
    char values[MAX_WRITE_SIZE][MAX_STRING_SIZE] = { 0 };
    unsigned int delay;
    size_t num_pairs;

    // printf("> "); // TODO se calhar tirar isto
    //  fflush(stdout); // e isto também

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
          fprintf(stderr, "Failed to write help to .out file\n");
          // TODO ver o que fazer
        }
        kvs_wait(delay);
      }
      break;

    case CMD_BACKUP:
      backup++;

      sem_wait(&sem);
      int pid = fork();
      if (pid < 0) { // error handling
        fprintf(stderr, "Failed to fork\n");
        sem_post(&sem); // liberta o semáforo
        return;

      } else if (pid == 0) { // child process

        printf("Backup %lu started\n", backup); // TODO remover
        if (kvs_backup(job_path, backup)) {
          fprintf(stderr, "Failed to perform backup.\n");
          exit(1);
        }
        kvs_terminate(); // TODO ver se é preciso terminar nos filhos, ver se é preciso free dir nos filhos e se calhar criar uma funcao para dar free em tudo
        sem_post(&sem);  // liberta o semáforo
        printf("Backup %lu done\n", backup); // TODO remover
        exit(0);

      } else {                    // parent process
        printf("next command\n"); // TODO remover

        // wait(NULL);
        sem_post(&sem); // liberta o semáforo
      }
      printf("next command outside\n"); // TODO remover
      break;

    case CMD_INVALID:
      fprintf(stderr, "Invalid command. See HELP for usage\n");
      break;

    case CMD_HELP:
      const char help[] = "Available commands:\n"
                          "  WRITE [(key,value)(key2,value2),...]\n"
                          "  READ [key,key2,...]\n"
                          "  DELETE [key,key2,...]\n"
                          "  SHOW\n"
                          "  WAIT <delay_ms>\n"
                          "  BACKUP\n" // Not implemented
                          "  HELP\n";
      if (write(out_fd, help, strlen(help)) != (ssize_t)strlen(help)) {
        fprintf(stderr, "Failed to write help to .out file\n");
        // TODO ver o que fazer
      }

      break;

    case CMD_EMPTY:
      break;

    case EOC:
      return;
    }
  }
}
