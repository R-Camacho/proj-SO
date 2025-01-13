#ifndef CLIENTS_H
#define CLIENTS_H

#include <semaphore.h>
#include <signal.h>

#include "constants.h"
#include "kvs.h"
#include "operations.h"
#include "src/common/constants.h"
#include "src/common/io.h"
#include "src/common/protocol.h"

#define SUBSCRIPTION_TABLE_SIZE 3517

typedef struct ClientNode {
  char *key;
  struct ClientNode *next;
} ClientNode;

typedef struct Client {
  int req_pipe_fd;
  int resp_pipe_fd;
  int notif_pipe_fd;
  int thread_active;
  int num_keys;
  ClientNode *keys;
  pthread_mutex_t client_lock;
} Client;

typedef struct ClientList {
  Client *clients[MAX_SESSION_COUNT];
  pthread_mutex_t list_lock;
  sem_t sem;
  sem_t session_slots;
} ClientList;

typedef struct SubscriptionNode {
  char *key;
  int notif_pipe_fds[MAX_SESSION_COUNT];
  struct SubscriptionNode *next;
} SubscriptionNode;

typedef struct SubscriptionTable {
  SubscriptionNode *table[SUBSCRIPTION_TABLE_SIZE];
  pthread_mutex_t tablelock;
} SubscriptionTable;


// Simple hash function for strings
// @param key String to hash
// @return Hash value
int hash_key(const char *key);

// Creates a new subscription table
// @return Newly created subscription table, NULL on failure
SubscriptionTable *create_subscription_table();

// Frees the subscription table
// @param ht Subscription table to be deleted
void free_subscription_table(SubscriptionTable *ht);

// Subscribes a client to a key
// @param ht Subscription table
// @param client Client to subscribe
// @param key Key to subscribe to
// @return 0 if successful, 1 if key was not found
int subscribe_key(SubscriptionTable *ht, Client *client, char *key);

// Unsubscribes a client from a key
// @param ht Subscription table
// @param client Client to unsubscribe
// @param key Key to unsubscribe from
// @return 0 if successful, 1 if key does not exist
int unsubscribe_key(SubscriptionTable *ht, Client *client, char *key);

// Notifies all clients subscribed to a key
// @param ht Subscription table
// @param key Key to notify clients
// @param value Value to notify clients
// @return 0 if successful, 1 if no client is subscribed to the key, -1 on error
int notify_clients(SubscriptionTable *ht, char *key, char *value);

// Initializes the client list
void init_client_list();

// Destroys the client list
void destroy_client_list();

// Host thread function for client threads
// @param arg Unused
void *read_register(void *arg);

// Responds to a client with a message
// @param client Client to respond to
// @param response Message to send
void respond_to_client(Client *client, char *response);

// Creates a new client
// @param request_message Message received from the client
// @return Newly created client, NULL on failure
Client *create_client(char *request_message);

// Inserts a client into the client list
// @param client Client to insert
void insert_client(Client *client);

// Removes a client from the client list
// @param client Client to remove
void remove_client(Client *client);

// Frees a client
void free_client(Client *client);

// Client thread function
void *client_thread(void *arg);

// Handler for SIGUSR1 signal
// only for host thread
void handle_sigusr1_signal();

#endif // CLIENTS_H