#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h> // TODO ver os includes depois
#include <time.h>
#include <unistd.h>

#include "constants.h"
#include "kvs.h"

static struct HashTable *kvs_table = NULL;


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
  return 0;
}

int kvs_write(size_t num_pairs, char keys[][MAX_STRING_SIZE], char values[][MAX_STRING_SIZE]) {
  if (kvs_table == NULL) {
    fprintf(stderr, "KVS state must be initialized\n");
    return 1;
  }

  for (size_t i = 0; i < num_pairs; i++) {
    if (write_pair(kvs_table, keys[i], values[i]) != 0) {
      fprintf(stderr, "Failed to write keypair (%s,%s)\n", keys[i], values[i]);
    }
  }

  return 0;
}

int kvs_read(size_t num_pairs, char keys[][MAX_STRING_SIZE], int fd) {
  if (kvs_table == NULL) {
    fprintf(stderr, "KVS state must be initialized\n");
    return 1;
  }

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
    }
    strncat(buffer, temp, strlen(temp));
    free(result);
  }
  strcat(buffer, "]\n");

  if (write(fd, buffer, strlen(buffer)) != (ssize_t)(strlen(buffer))) {
    fprintf(stderr, "Failed to write to .out file\n");
    kvs_terminate(); // TODO ver se é preciso terminar ou basta dar return/continue/exit
    return 1;
    //  TODO exit(1);
  }
  free(buffer);

  return 0;
}

int kvs_delete(size_t num_pairs, char keys[][MAX_STRING_SIZE], int fd) {
  if (kvs_table == NULL) {
    fprintf(stderr, "KVS state must be initialized\n");
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
    kvs_terminate(); // TODO ver se é preciso terminar ou basta dar return/continue/exit
    return 1;
    //  TODO exit(1);
  }
  free(buffer);

  return 0;
}

void kvs_show(int fd) {
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
    kvs_terminate(); // TODO ver se é preciso terminar ou basta dar return/continue/exit
    // return 1;
    //  TODO exit(1);
  }
  free(buffer);
}

// TODO
int kvs_backup() {
  return 0;
}

void kvs_wait(unsigned int delay_ms) {
  struct timespec delay = delay_to_timespec(delay_ms);
  nanosleep(&delay, NULL);
}