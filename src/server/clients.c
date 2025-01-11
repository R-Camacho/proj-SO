#include "clients.h"

ClientList *client_list;


SubscriptionTable *create_subscription_table() {
  SubscriptionTable *ht = (SubscriptionTable *)malloc(sizeof(SubscriptionTable));
  if (!ht) return NULL;
  for (int i = 0; i < SUBSCRIPTION_TABLE_SIZE; i++) {
    ht->table[i] = NULL;
  }
  pthread_mutex_init(&ht->tablelock, NULL);
  return ht;
}

void free_subscription_table(SubscriptionTable *ht) {
  for (size_t i = 0; i < SUBSCRIPTION_TABLE_SIZE; i++) {
    ClientKeyNode *current = ht->table[i];
    while (current != NULL) {
      ClientKeyNode *next = current->next;
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

void insert_key(SubscriptionTable *ht, Client *client, char *key) {
  int index = hash_key(key);
  pthread_mutex_lock(&client->client_lock);

  ClientKeyNode *keyNode = ht->table[index];
  ClientKeyNode *previousNode;
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
      return;
    }
    previousNode = keyNode;
    keyNode      = previousNode->next; // Move to the next node
  }
  // Key not found, create a new key node
  keyNode      = (ClientKeyNode *)malloc(sizeof(ClientKeyNode));
  keyNode->key = strdup(key);

  // initialize all pipe_fds at -1
  for (size_t i = 1; i < MAX_SESSION_COUNT; i++) {
    keyNode->notif_pipe_fds[i] = -1;
  }
  keyNode->notif_pipe_fds[0] = client->notif_pipe_fd;
  keyNode->next              = ht->table[index];
  ht->table[index]           = keyNode;

  pthread_mutex_unlock(&client->client_lock);
}


void init_client_list(ClientList *list) {
  list->n_clients = 0;
  pthread_mutex_init(&list->list_lock, NULL);
  sem_init(&list->sem, 0, MAX_SESSION_COUNT);
}

void destroy_client_list(ClientList *list) {
  for (size_t i = 0; i < MAX_SESSION_COUNT; i++) {
    free_client(list->clients[i]);
  }
  pthread_mutex_destroy(&list->list_lock);
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

  int register_pipe_fd;
  if ((register_pipe_fd = open_file(register_pipe_path, O_RDONLY)) == -1) {
    write_all(STDERR_FILENO, "Failed to open register pipe\n", 29);
    return NULL;
  }
  char buffer[MAX_REGISTER_LENGTH];
  int intr = 0;

  while (1) {
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

    write_all(STDOUT_FILENO, buffer, strlen(buffer)); // TODO tirar

    // se a lista de clients tiver cheia, esperar
    sem_wait(&client_list->sem);


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

// TODO VERIFICAR MAX KEYS CLIENT


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

  char *req_pipe_p   = "";
  char *resp_pipe_p  = "";
  char *notif_pipe_p = "";
  strncat(req_pipe_p, 1 + request_message, MAX_PIPE_PATH_LENGTH);
  strncat(resp_pipe_p, 1 + MAX_PIPE_PATH_LENGTH + request_message, MAX_PIPE_PATH_LENGTH);
  strncat(notif_pipe_p, 1 + 2 * MAX_PIPE_PATH_LENGTH + request_message, MAX_PIPE_PATH_LENGTH);

  client->req_pipe_fd   = open_file(req_pipe_p, O_RDONLY);
  client->resp_pipe_fd  = open_file(resp_pipe_p, O_WRONLY);
  client->notif_pipe_fd = open_file(notif_pipe_p, O_WRONLY);

  if (client->req_pipe_fd == -1 || client->resp_pipe_fd == -1 || client->notif_pipe_fd == -1) {
    free(client);
    return NULL;
  }


  client->keys = NULL;
  pthread_mutex_init(&client->client_lock, NULL);

  return client;
}

void insert_client(Client *client) {
  pthread_mutex_lock(&client_list->list_lock);
  if (client_list->n_clients == MAX_SESSION_COUNT) { // should never happen
    pthread_mutex_unlock(&client_list->list_lock);
    return;
  }
  client_list->clients[client_list->n_clients++] = client;
  pthread_mutex_unlock(&client_list->list_lock);
}

void remove_client(Client *client) {
  pthread_mutex_lock(&client_list->list_lock);
  for (size_t i = 0; i < client_list->n_clients; i++) {
    if (client_list->clients[i] == client) {
      free_client(client_list->clients[i]);
      client_list->clients[i] = client_list->clients[client_list->n_clients - 1];
      client_list->n_clients--;
      break;
    }
  }
  sem_post(&client_list->sem);
  pthread_mutex_unlock(&client_list->list_lock);
}

void *client_thread(void *arg) {
  int index = *(int *)arg;

  (void)index; // TODO unused


  return NULL;
}
