#ifndef CLIENTS_H
#define CLIENTS_H

#include "constants.h"
#include "kvs.h"
#include "src/common/constants.h"
#include "src/common/io.h"
#include "src/common/protocol.h"
#include <semaphore.h>

#define SUBSCRIPTION_TABLE_SIZE 3517

typedef struct ClientNode {
  char *key;
  struct ClientNode *next;
} ClientNode;

typedef struct Client {
  int req_pipe_fd;
  int resp_pipe_fd;
  int notif_pipe_fd;
  ClientNode *keys;
  pthread_mutex_t client_lock;
} Client;

typedef struct ClientList {
  Client *clients[MAX_SESSION_COUNT];
  size_t n_clients;
  pthread_mutex_t list_lock; // nao sei se é preciso
  sem_t sem;
} ClientList;

typedef struct ClientKeyNode {
  char *key;
  int notif_pipe_fds[MAX_SESSION_COUNT];
  struct ClientKeyNode *next;
} ClientKeyNode;

typedef struct SubscriptionTable {
  ClientKeyNode *table[TABLE_SIZE];
  pthread_mutex_t tablelock;
} SubscriptionTable;


// TODO add comments

int hash_key(const char *key);

SubscriptionTable *create_subscription_table();

void free_subscription_table(SubscriptionTable *ht);

void init_client_list(ClientList *list);

void *client_thread(void *arg);

void *read_register(void *arg);

void respond_to_client(Client *client, char *response);

Client *create_client(char *request_message);

void insert_client(Client *client);

void remove_client(Client *client);

void free_client(Client *client);

void *client_thread(void *arg);

#endif // CLIENTS_H