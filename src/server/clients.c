#include "clients.h"

extern struct HashTable *kvs_table;
extern struct SubscriptionTable *subscription_table;

extern volatile sig_atomic_t sigusr1;

ClientList *client_list = NULL;


SubscriptionTable *create_subscription_table() {
  SubscriptionTable *ht = (SubscriptionTable *)malloc(sizeof(SubscriptionTable));
  if (!ht) return NULL;
  for (size_t i = 0; i < SUBSCRIPTION_TABLE_SIZE; i++) {
    ht->table[i] = NULL;
  }
  pthread_mutex_init(&ht->tablelock, NULL);
  return ht;
}

void free_subscription_table(SubscriptionTable *ht) {
  for (size_t i = 0; i < SUBSCRIPTION_TABLE_SIZE; i++) {
    SubscriptionNode *current = ht->table[i];
    while (current != NULL) {
      SubscriptionNode *next = current->next;
      free(current->key);
      free(current);
      current = next;
    }
  }
}

int hash_key(const char *str) {
  int hash = 5381; // seed
  int c;
  while ((c = *str++))
    hash = ((hash << 5) + hash) + c;

  return hash % SUBSCRIPTION_TABLE_SIZE;
}

int subscribe_key(SubscriptionTable *ht, Client *client, char *key) {
  int index = hash_key(key);

  pthread_rwlock_rdlock(&kvs_table->tablelock);
  if (read_pair(kvs_table, key) == NULL) {
    return 1; // key not found in the kvs_table
  }
  pthread_rwlock_unlock(&kvs_table->tablelock);

  if (client->num_keys >= MAX_SESSION_COUNT) {
    return 1; // client already subscribed to the maximum number of keys
  }
  // Add key to the end of client key list
  pthread_mutex_lock(&client->client_lock);
  ClientNode *newNode = (ClientNode *)malloc(sizeof(ClientNode));
  newNode->key        = strdup(key);
  newNode->next       = NULL;
  if (client->keys == NULL) {
    client->keys = newNode;
  } else {
    ClientNode *current = client->keys;
    while (current->next != NULL) {
      current = current->next;
    }
    current->next = newNode;
  }
  pthread_mutex_unlock(&client->client_lock);

  pthread_mutex_lock(&ht->tablelock);
  SubscriptionNode *keyNode = ht->table[index];
  SubscriptionNode *previousNode;
  while (keyNode != NULL) {
    if (strcmp(keyNode->key, key) == 0) { // key already exists
      // add client to the list
      int notif_pipe_fd = client->notif_pipe_fd;
      for (size_t i = 0; i < MAX_SESSION_COUNT; i++) {
        if (keyNode->notif_pipe_fds[i] == -1) {
          keyNode->notif_pipe_fds[i] = notif_pipe_fd;
          break;
        }
      }
      client->num_keys++;
      pthread_mutex_unlock(&ht->tablelock);
      return 0;
    }
    previousNode = keyNode;
    keyNode      = previousNode->next; // Move to the next node
  }
  // Key not found, create a new key node
  keyNode      = (SubscriptionNode *)malloc(sizeof(SubscriptionNode));
  keyNode->key = strdup(key);

  // initialize all pipe_fds at -1
  for (size_t i = 1; i < MAX_SESSION_COUNT; i++) {
    keyNode->notif_pipe_fds[i] = -1;
  }
  keyNode->notif_pipe_fds[0] = client->notif_pipe_fd;
  keyNode->next              = ht->table[index];
  ht->table[index]           = keyNode;
  client->num_keys++;

  pthread_mutex_unlock(&ht->tablelock);
  return 0;
}

void unsubscribe_all_clients(SubscriptionTable *ht, char *key) {
  for (size_t i = 0; i < MAX_SESSION_COUNT; i++) {
    if (client_list->clients[i] != NULL) {
      unsubscribe_key(ht, client_list->clients[i], key);
    }
  }
}

// returns 0 if key was successfully unsubscribed, 1 if key was not found
int unsubscribe_key(SubscriptionTable *ht, Client *client, char *key) {
  int index = hash_key(key);

  // Remove from client key list
  pthread_mutex_lock(&client->client_lock);
  ClientNode *current = client->keys;
  ClientNode *prev    = NULL;

  while (current != NULL) {
    if (strcmp(current->key, key) == 0) {
      if (prev == NULL) {
        client->keys = current->next;
      } else {
        prev->next = current->next;
      }
      client->num_keys--;
      break;
    }
    prev    = current;
    current = current->next;
  }
  pthread_mutex_unlock(&client->client_lock);

  pthread_mutex_lock(&ht->tablelock);
  SubscriptionNode *keyNode = ht->table[index];
  while (keyNode != NULL) {
    if (strcmp(keyNode->key, key) == 0) { // key found
      // remove client from the list
      int client_fd = client->notif_pipe_fd;
      int found     = 0;
      for (size_t i = 0; i < MAX_SESSION_COUNT; i++) {
        if (keyNode->notif_pipe_fds[i] == client_fd) {
          keyNode->notif_pipe_fds[i] = -1;

          found = 1;
          break;
        }
      }
      pthread_mutex_unlock(&ht->tablelock);
      if (found)
        return 0; // key successfully unsubscribed
      else
        return 1; // client was not subscribed to this key
    }
    keyNode = keyNode->next; // Move to the next node
  }

  pthread_mutex_unlock(&ht->tablelock);
  return 1; // key not found
}

int notify_clients(SubscriptionTable *ht, char *key, char *value) {
  int index = hash_key(key);

  SubscriptionNode *keyNode = ht->table[index];

  while (keyNode != NULL) {
    if (strcmp(keyNode->key, key) == 0) {
      size_t key_length   = strlen(key);
      size_t value_length = strlen(value);

      char notification[MAX_NOTIFICATION_SIZE] = { 0 };
      strncpy(notification, "(", 2);
      strncpy(notification + 1, key, key_length);
      strncpy(notification + 1 + key_length, ",", 2);
      strncpy(notification + 1 + key_length + 1, value, value_length);
      strncpy(notification + 1 + key_length + 1 + value_length, ")", 2);


      for (size_t i = 0; i < MAX_SESSION_COUNT; i++) {
        if (keyNode->notif_pipe_fds[i] != -1) {
          if (write_all(keyNode->notif_pipe_fds[i], notification, MAX_NOTIFICATION_SIZE) == -1) return -1;
        }
      }
      return 0;
    }
    keyNode = keyNode->next;
  }
  return 1; // no client subscribed to this key
}

void init_client_list() {
  client_list = (ClientList *)malloc(sizeof(ClientList));
  for (size_t i = 0; i < MAX_SESSION_COUNT; i++) {
    client_list->clients[i] = NULL;
  }
  pthread_mutex_init(&client_list->list_lock, NULL);
  sem_init(&client_list->sem, 0, 0);
  sem_init(&client_list->session_slots, 0, MAX_SESSION_COUNT);
}

void destroy_client_list() {
  for (size_t i = 0; i < MAX_SESSION_COUNT; i++) {
    free_client(client_list->clients[i]);
  }
  sem_destroy(&client_list->sem);
  sem_destroy(&client_list->session_slots);
  pthread_mutex_destroy(&client_list->list_lock);
}

void free_client(Client *client) {
  ClientNode *current = client->keys;
  while (current != NULL) {
    ClientNode *next = current->next;
    free(current->key);
    free(current);
    current = next;
  }
  pthread_mutex_destroy(&client->client_lock);
  free(client);
}

void *read_register(void *arg) {
  char *register_pipe_path = (char *)arg;

  // Unblock SIGUSR1 only in this thread
  sigset_t set;
  sigemptyset(&set);
  sigaddset(&set, SIGUSR1);
  if (pthread_sigmask(SIG_UNBLOCK, &set, NULL) != 0) {
    fprintf(stderr, "Failed to unblock SIGUSR1\n");
  }

  int register_pipe_fd;
  if ((register_pipe_fd = open_file(register_pipe_path, O_RDONLY)) == -1) {
    write_str(STDERR_FILENO, "Failed to open register pipe\n");
    return NULL;
  }
  char buffer[MAX_REGISTER_LENGTH];
  int intr = 0;

  while (1) {

    // Check if SIGUSR1 was received
    if (sigusr1) {
      handle_sigusr1_signal();
      sigusr1 = 0;
      continue;
    }

    // clear buffer
    memset(buffer, 0, sizeof(buffer));

    // read notification
    ssize_t bytes_read = read_all(register_pipe_fd, buffer, sizeof(buffer), &intr);
    if (bytes_read == -1) {
      if (errno == EAGAIN || errno == EWOULDBLOCK) {
        // No data to read
        continue;
      }
      if (intr) {
        fprintf(stderr, "Read interrupted\n");
        break;
      }
      // Other error occurred
      fprintf(stderr, "Failed to read from notification pipe: %s\n", strerror(errno));
      continue;
    }

    if (bytes_read == 0) {
      continue;
    }

    size_t len = strlen(buffer);
    if (len == 0) continue; // this may never happen
    if (buffer[0] != OP_CODE_CONNECT) {
      write_str(STDERR_FILENO, "Invalid request\n");
      continue;
    }

    sem_wait(&client_list->session_slots); // wait for a slot to be available

    Client *client = create_client(buffer);
    insert_client(client);

    // response
    char response[2];
    response[0] = OP_CODE_CONNECT;
    response[1] = '0';
    respond_to_client(client, response);
  }

  return NULL;
}

void respond_to_client(Client *client, char *response) {
  int resp_pipe_fd = client->resp_pipe_fd;
  if (resp_pipe_fd == -1) {
    write_all(STDERR_FILENO, "Failed to open response pipe\n", 29);
    return;
  }
  if (write_all(resp_pipe_fd, response, 2) == -1) {
    write_all(STDERR_FILENO, "Failed to write to response pipe\n", 33);
    return;
  }
}

Client *create_client(char *request_message) {
  Client *client = (Client *)malloc(sizeof(Client));
  if (client == NULL) {
    return NULL;
  }

  char req_pipe_p[MAX_PIPE_PATH_LENGTH + 1]   = { 0 }; // +1 for null terminator
  char resp_pipe_p[MAX_PIPE_PATH_LENGTH + 1]  = { 0 };
  char notif_pipe_p[MAX_PIPE_PATH_LENGTH + 1] = { 0 };

  strncpy(req_pipe_p, request_message + 1, MAX_PIPE_PATH_LENGTH);
  strncpy(resp_pipe_p, request_message + 1 + MAX_PIPE_PATH_LENGTH, MAX_PIPE_PATH_LENGTH);
  strncpy(notif_pipe_p, request_message + 1 + (2 * MAX_PIPE_PATH_LENGTH), MAX_PIPE_PATH_LENGTH);

  client->req_pipe_fd   = open_file(req_pipe_p, O_RDONLY);
  client->resp_pipe_fd  = open_file(resp_pipe_p, O_WRONLY);
  client->notif_pipe_fd = open_file(notif_pipe_p, O_WRONLY);

  if (client->req_pipe_fd == -1 || client->resp_pipe_fd == -1 || client->notif_pipe_fd == -1) {
    free(client);
    return NULL;
  }

  client->num_keys      = 0;
  client->thread_active = 0;
  client->keys          = NULL;
  pthread_mutex_init(&client->client_lock, NULL);

  return client;
}

void insert_client(Client *client) {
  pthread_mutex_lock(&client_list->list_lock);

  // find empty slot
  for (size_t i = 0; i < MAX_SESSION_COUNT; i++) {
    if (client_list->clients[i] == NULL) {
      client_list->clients[i] = client;
      break;
    }
  }

  pthread_mutex_unlock(&client_list->list_lock);
  sem_post(&client_list->sem);
}

void remove_client(Client *client) {

  for (size_t i = 0; i < MAX_SESSION_COUNT; i++) {
    if (client_list->clients[i] == client) { // client found

      // unsubscribe client from all keys
      ClientNode *keyNode = client->keys;
      while (keyNode != NULL) {
        unsubscribe_key(subscription_table, client, keyNode->key);
        keyNode = keyNode->next;
      }
      if (close_file(client->req_pipe_fd) == -1) {
        write_str(STDERR_FILENO, "Failed to close request pipe\n");
        free_client(client_list->clients[i]);
        break;
      }
      if (close_file(client->resp_pipe_fd) == -1) {
        write_str(STDERR_FILENO, "Failed to close response pipe\n");
        free_client(client_list->clients[i]);
        break;
      }
      if (close_file(client->notif_pipe_fd) == -1) {
        write_str(STDERR_FILENO, "Failed to close notification pipe\n");
        free_client(client_list->clients[i]);
        break;
      }
      free_client(client_list->clients[i]);
      client_list->clients[i] = NULL;
      break;
    }
  }
  sem_post(&client_list->session_slots);
}

void *client_thread(void *arg) {
  (void)arg; // unused

  // Ignore SIGUSR1
  sigset_t set;
  sigemptyset(&set);
  sigaddset(&set, SIGUSR1);
  if (pthread_sigmask(SIG_BLOCK, &set, NULL) != 0) {
    fprintf(stderr, "Failed to mask SIGUSR1\n");
  }

  while (1) {
    sem_wait(&client_list->sem); // wait for a client to connect

    pthread_mutex_lock(&client_list->list_lock);
    // find first free client
    Client *client = NULL;
    for (size_t i = 0; i < MAX_SESSION_COUNT; i++) {
      if (client_list->clients[i] != NULL && client_list->clients[i]->thread_active == 0) {
        client_list->clients[i]->thread_active = 1;
        client                                 = client_list->clients[i];
        break;
      }
    }
    pthread_mutex_unlock(&client_list->list_lock);

    if (!client) continue;

    // Process client requests
    char buffer[1 + MAX_STRING_SIZE + 1] = { 0 };
    int intr                             = 0;

    while (1) {
      // clear buffer
      memset(buffer, 0, sizeof(buffer));

      ssize_t bytes_read = read_all(client->req_pipe_fd, buffer, sizeof(buffer), &intr);
      if (bytes_read == -1) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
          // No data to read
          continue;
        }
        if (intr) {
          fprintf(stderr, "Read interrupted\n");
          break;
        }
        // Other error occurred
        fprintf(stderr, "Failed to read from notification pipe: %s\n", strerror(errno));
        break;
      }

      if (bytes_read == 0) {
        continue;
      }

      char response[2] = { 0 };
      switch (buffer[0]) {
      case OP_CODE_SUBSCRIBE:
        response[0] = OP_CODE_SUBSCRIBE;
        if (subscribe_key(subscription_table, client, buffer + 1) == 1)
          response[1] = '0'; // key was not found
        else
          response[1] = '1'; // key was successfully subscribed

        respond_to_client(client, response);
        break;
      case OP_CODE_UNSUBSCRIBE:
        response[0] = OP_CODE_UNSUBSCRIBE;
        if (unsubscribe_key(subscription_table, client, buffer + 1) == 1)
          response[1] = '1'; // key was not found
        else
          response[1] = '0'; // key was successfully unsubscribed
        respond_to_client(client, response);
        break;
      case OP_CODE_DISCONNECT:
        response[0] = OP_CODE_DISCONNECT;
        response[1] = '0';
        respond_to_client(client, response);
        pthread_mutex_lock(&client_list->list_lock);
        remove_client(client);
        pthread_mutex_unlock(&client_list->list_lock);
        break;
      }

      if (buffer[0] == OP_CODE_DISCONNECT) {
        break;
      }
    }
  }

  return NULL;
}


void handle_sigusr1_signal() {
  pthread_mutex_lock(&client_list->list_lock);
  for (size_t i = 0; i < MAX_SESSION_COUNT; i++) {
    if (client_list->clients[i] != NULL) {
      remove_client(client_list->clients[i]);
    }
  }
  pthread_mutex_unlock(&client_list->list_lock);
}