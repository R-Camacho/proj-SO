#include "operations.h"

static struct HashTable *kvs_table = NULL;

pthread_rwlock_t kvs_lock = PTHREAD_RWLOCK_INITIALIZER;

/// Calculates a timespec from a delay in milliseconds.
/// @param delay_ms Delay in milliseconds.
/// @return Timespec with the given delay.
static struct timespec delay_to_timespec(unsigned int delay_ms) {
  return (struct timespec){ delay_ms / 1000, (delay_ms % 1000) * 1000000 };
}

int kvs_init() {
  if (kvs_table != NULL) {
    fprintf(stderr, "KVS state has already been initialized\n");
    return 1;
  }

  kvs_table = create_hash_table();
  return kvs_table == NULL;
}

int kvs_terminate() {
  if (kvs_table == NULL) {
    fprintf(stderr, "KVS state must be initialized\n");
    return 1;
  }

  free_table(kvs_table);
  kvs_table = NULL;
  return 0;
}

int kvs_write(size_t num_pairs, char keys[][MAX_STRING_SIZE], char values[][MAX_STRING_SIZE]) {
  if (kvs_table == NULL) {
    fprintf(stderr, "KVS state must be initialized\n");
    return 1;
  }

  // sort keys and values lexicographically (using pair comparator)
  KVPair *pairs = (KVPair *)malloc(num_pairs * sizeof(KVPair));
  for (size_t i = 0; i < num_pairs; i++) {
    pairs[i].key   = keys[i];
    pairs[i].value = values[i];
  }
  qsort(pairs, num_pairs, sizeof(KVPair), (int (*)(const void *, const void *))kv_pair_comparator);


  pthread_rwlock_wrlock(&kvs_lock);

  for (size_t i = 0; i < num_pairs; i++) {
    if (write_pair(kvs_table, pairs[i].key, pairs[i].value) != 0) {
      fprintf(stderr, "Failed to write keypair (%s,%s)\n", pairs[i].key, pairs[i].value);
    }
  }
  pthread_rwlock_unlock(&kvs_lock);

  free(pairs);
  return 0;
}

int kvs_read(size_t num_pairs, char keys[][MAX_STRING_SIZE], int fd) {
  if (kvs_table == NULL) {
    fprintf(stderr, "KVS state must be initialized\n");
    return 1;
  }

  // sort keys lexicographically (using strcmp comparator)
  qsort(keys, num_pairs, MAX_STRING_SIZE, (int (*)(const void *, const void *))strcmp);

  pthread_rwlock_rdlock(&kvs_lock); // no one can write into the hash table

  char temp[MAX_WRITE_SIZE];
  char *buffer = (char *)malloc(MAX_WRITE_SIZE * sizeof(char));
  memset(buffer, 0, sizeof(*buffer));
  *buffer = '\0';

  strcat(buffer, "[");
  for (size_t i = 0; i < num_pairs; i++) {
    char *result = read_pair(kvs_table, keys[i]);
    if (result == NULL) {
      snprintf(temp, MAX_WRITE_SIZE, "(%s,KVSERROR)", keys[i]);
    } else {
      snprintf(temp, MAX_WRITE_SIZE, "(%s,%s)", keys[i], result);
      free(result);
    }
    strncat(buffer, temp, strlen(temp));
  }
  strcat(buffer, "]\n");

  pthread_rwlock_unlock(&kvs_lock); // allow writing again

  if (write(fd, buffer, strlen(buffer)) != (ssize_t)(strlen(buffer))) {
    fprintf(stderr, "Failed to write to .out file\n");
    return 1;
  }
  free(buffer);
  return 0;
}

int kvs_delete(size_t num_pairs, char keys[][MAX_STRING_SIZE], int fd) {
  if (kvs_table == NULL) {
    fprintf(stderr, "KVS state must be initialized\n");
    return 1;
  }
  // sort keys lexicographically (using strcmp comparator)
  qsort(keys, num_pairs, MAX_STRING_SIZE, (int (*)(const void *, const void *))strcmp);

  pthread_rwlock_wrlock(&kvs_lock);
  int aux = 0;

  char temp[MAX_WRITE_SIZE];
  char *buffer = (char *)malloc(MAX_WRITE_SIZE * sizeof(char));
  memset(buffer, 0, sizeof(*buffer));
  *buffer = '\0';

  for (size_t i = 0; i < num_pairs; i++) {
    if (delete_pair(kvs_table, keys[i]) != 0) {
      if (!aux) {
        strcat(buffer, "[");
        aux = 1;
      }
      snprintf(temp, MAX_WRITE_SIZE, "(%s,KVSMISSING)", keys[i]);
      strncat(buffer, temp, strlen(temp));
    }
  }
  if (aux) {
    strcat(buffer, "]\n");
  }
  pthread_rwlock_unlock(&kvs_lock);

  if (write(fd, buffer, strlen(buffer)) != (ssize_t)(strlen(buffer))) {
    fprintf(stderr, "Failed to write to .out file\n");
    free(buffer);
    return 1;
  }
  free(buffer);
  return 0;
}

void kvs_show(int fd) {
  if (kvs_table == NULL) {
    fprintf(stderr, "KVS state must be initialized\n");
    return;
  }
  pthread_rwlock_rdlock(&kvs_lock); // no one can write into the hash table

  char temp[MAX_WRITE_SIZE];
  char *buffer = (char *)malloc(MAX_WRITE_SIZE * sizeof(char));
  memset(buffer, 0, sizeof(*buffer));
  *buffer = '\0';

  for (int i = 0; i < TABLE_SIZE; i++) {
    KeyNode *keyNode = kvs_table->table[i];
    while (keyNode != NULL) {
      snprintf(temp, MAX_WRITE_SIZE, "(%s, %s)\n", keyNode->key, keyNode->value);
      strncat(buffer, temp, strlen(temp));

      keyNode = keyNode->next; // Move to the next node
    }
  }

  if (write(fd, buffer, strlen(buffer)) != (ssize_t)(strlen(buffer))) {
    fprintf(stderr, "Failed to write to .out file\n");
    free(buffer);
    return;
  }

  free(buffer);
  pthread_rwlock_unlock(&kvs_lock);
}

int kvs_backup(const char *job_path, size_t backup) {

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
    return 1;
  }
  return 0;
}

void kvs_wait(unsigned int delay_ms) {
  struct timespec delay = delay_to_timespec(delay_ms);
  nanosleep(&delay, NULL);
}

int kv_pair_comparator(KVPair *a, KVPair *b) {
  return strcmp(a->key, b->key);
}
