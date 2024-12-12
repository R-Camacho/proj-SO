#include "operations.h"

static struct HashTable *kvs_table = NULL;

static pthread_mutex_t kvs_mutex = PTHREAD_MUTEX_INITIALIZER;

pthread_mutex_t backup_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t backup_cond   = PTHREAD_COND_INITIALIZER;
size_t active_backups        = 0;


/// Calculates a timespec from a delay in milliseconds.
/// @param delay_ms Delay in milliseconds.
/// @return Timespec with the given delay.
static struct timespec delay_to_timespec(unsigned int delay_ms) {
  return (struct timespec){ delay_ms / 1000, (delay_ms % 1000) * 1000000 };
}

int kvs_init() {
  pthread_mutex_lock(&kvs_mutex);
  if (kvs_table != NULL) {
    fprintf(stderr, "KVS state has already been initialized\n");
    pthread_mutex_unlock(&kvs_mutex);
    return 1;
  }

  kvs_table = create_hash_table();
  pthread_mutex_unlock(&kvs_mutex);
  return kvs_table == NULL;
}

int kvs_terminate() {
  pthread_mutex_lock(&kvs_mutex);
  if (kvs_table == NULL) {
    fprintf(stderr, "KVS state must be initialized\n");
    pthread_mutex_unlock(&kvs_mutex);
    return 1;
  }

  free_table(kvs_table);
  kvs_table = NULL;
  pthread_mutex_unlock(&kvs_mutex);
  return 0;
}

int kvs_write(size_t num_pairs, char keys[][MAX_STRING_SIZE], char values[][MAX_STRING_SIZE]) {
  pthread_mutex_lock(&kvs_mutex);
  if (kvs_table == NULL) {
    fprintf(stderr, "KVS state must be initialized\n");
    pthread_mutex_unlock(&kvs_mutex);
    return 1;
  }

  // sort keys lexicographically (using strcmp comparator)
  qsort(keys, num_pairs, MAX_STRING_SIZE, (int (*)(const void *, const void *))strcmp);

  for (size_t i = 0; i < num_pairs; i++) {
    if (write_pair(kvs_table, keys[i], values[i]) != 0) {
      fprintf(stderr, "Failed to write keypair (%s,%s)\n", keys[i], values[i]);
    }
  }
  pthread_mutex_unlock(&kvs_mutex);
  return 0;
}

int kvs_read(size_t num_pairs, char keys[][MAX_STRING_SIZE], int fd) {
  pthread_mutex_lock(&kvs_mutex);
  if (kvs_table == NULL) {
    fprintf(stderr, "KVS state must be initialized\n");
    pthread_mutex_unlock(&kvs_mutex);
    return 1;
  }

  // sort keys lexicographically (using strcmp comparator)
  qsort(keys, num_pairs, MAX_STRING_SIZE, (int (*)(const void *, const void *))strcmp);

  char temp[MAX_WRITE_SIZE]; // TODO verificar constante
  char *buffer = (char *)malloc(MAX_WRITE_SIZE * sizeof(char));
  memset(buffer, 0, sizeof(*buffer));
  *buffer = '\0';

  strcat(buffer, "[");
  for (size_t i = 0; i < num_pairs; i++) {
    char *result = read_pair(kvs_table, keys[i]);
    if (result == NULL) {
      snprintf(temp, MAX_WRITE_SIZE, "(%s,KVSERROR)", keys[i]); // TODO verificar constante
    } else {
      snprintf(temp, MAX_WRITE_SIZE, "(%s,%s)", keys[i], result);
      free(result);
    }
    strncat(buffer, temp, strlen(temp));
  }
  strcat(buffer, "]\n");

  if (write(fd, buffer, strlen(buffer)) != (ssize_t)(strlen(buffer))) {
    fprintf(stderr, "Failed to write to .out file\n");
    pthread_mutex_unlock(&kvs_mutex);
  }
  free(buffer);
  pthread_mutex_unlock(&kvs_mutex);
  return 0;
}

int kvs_delete(size_t num_pairs, char keys[][MAX_STRING_SIZE], int fd) {
  pthread_mutex_lock(&kvs_mutex);
  if (kvs_table == NULL) {
    fprintf(stderr, "KVS state must be initialized\n");
    pthread_mutex_unlock(&kvs_mutex);
    return 1;
  }
  int aux = 0;

  char temp[MAX_WRITE_SIZE];                                    // TODO verificar constante
  char *buffer = (char *)malloc(MAX_WRITE_SIZE * sizeof(char)); // TODO verificar constante
  memset(buffer, 0, sizeof(*buffer));
  *buffer = '\0';

  for (size_t i = 0; i < num_pairs; i++) {
    if (delete_pair(kvs_table, keys[i]) != 0) {
      if (!aux) {
        strcat(buffer, "[");
        aux = 1;
      }
      snprintf(temp, MAX_WRITE_SIZE, "(%s,KVSMISSING)", keys[i]); // TODO verificar constante
      strncat(buffer, temp, strlen(temp));
    }
  }
  if (aux) {
    strcat(buffer, "]\n");
  }

  if (write(fd, buffer, strlen(buffer)) != (ssize_t)(strlen(buffer))) {
    fprintf(stderr, "Failed to write to .out file\n");
    pthread_mutex_unlock(&kvs_mutex);
    free(buffer);
    return 1;
    //  TODO exit(1);
  }
  free(buffer);
  pthread_mutex_unlock(&kvs_mutex);
  return 0;
}

void kvs_show(int fd) {
  pthread_mutex_lock(&kvs_mutex);
  if (kvs_table == NULL) {
    fprintf(stderr, "KVS state must be initialized\n");
    pthread_mutex_unlock(&kvs_mutex);
    return;
  }
  char temp[MAX_WRITE_SIZE];                                    // TODO verificar constante
  char *buffer = (char *)malloc(MAX_WRITE_SIZE * sizeof(char)); // TODO verificar constante
  memset(buffer, 0, sizeof(*buffer));
  *buffer = '\0';

  for (int i = 0; i < TABLE_SIZE; i++) {
    KeyNode *keyNode = kvs_table->table[i];
    while (keyNode != NULL) {
      snprintf(temp, MAX_WRITE_SIZE, "(%s, %s)\n", keyNode->key, keyNode->value); // TODO verificar constante
      strncat(buffer, temp, strlen(temp));

      keyNode = keyNode->next; // Move to the next node
    }
  }
  if (write(fd, buffer, strlen(buffer)) != (ssize_t)(strlen(buffer))) {
    fprintf(stderr, "Failed to write to .out file\n");
    pthread_mutex_unlock(&kvs_mutex);
    free(buffer);
    return;
    //  TODO exit(1);
  }
  free(buffer);
  pthread_mutex_unlock(&kvs_mutex);
}

// TODO
int kvs_backup(const char *job_path, size_t backup) {
  // TODO se calhar criar uma macro no constants.h para estas duas
  int open_flags = O_CREAT | O_WRONLY | O_TRUNC;
  // rw-rw-rw (or 0666)
  mode_t file_perms = S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH;

  // Create correspondent .bck file <nome-ficheiro>-<num-backup>.bck
  char bck_path[PATH_MAX] = "";
  strncpy(bck_path, job_path, strlen(job_path) - 4); // remove ".job"
  // bck_path += "-<num-backup>.bck"
  snprintf(bck_path + strlen(bck_path), PATH_MAX - strlen(bck_path), "-%lu.bck", backup);

  int bck_fd = open(bck_path, open_flags, file_perms);
  if (bck_fd < 0) {
    fprintf(stderr, "Failed to open .bck file\n");
    return 1;
  }

  kvs_show(bck_fd);

  if (close(bck_fd) < 0) {
    fprintf(stderr, "Failed to close .out file\n");
    kvs_terminate();
  }
  return 0;
}

void kvs_wait(unsigned int delay_ms) {
  struct timespec delay = delay_to_timespec(delay_ms);
  nanosleep(&delay, NULL);
}