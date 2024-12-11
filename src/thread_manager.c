#include "thread_manager.h"

t_manager *thread_manager = NULL;

int file_queue_init(FileQueue *queue) {
  if (queue == NULL) {
    fprintf(stderr, "Queue pointer is NULL\n");
    return 1;
  }
  queue->head = queue->tail = NULL;
  pthread_mutex_init(&queue->mutex, NULL);
  pthread_cond_init(&queue->cond, NULL);

  return 0;
}

int file_queue_destroy(FileQueue *queue) {
  if (queue == NULL) {
    fprintf(stderr, "File queue cannot be destroyed\n");
    return 1;
  }

  FileNode *current = queue->head;
  while (current != NULL) {
    FileNode *temp = current;
    current        = current->next;
    free(temp->file_path);
    free(temp);
  }
  pthread_mutex_destroy(&queue->mutex);
  pthread_cond_destroy(&queue->cond);

  return 0;
}

void file_queue_push(FileQueue *queue, const char *file_path) {
  FileNode *node = (FileNode *)malloc(sizeof(FileNode));
  if (node == NULL) {
    fprintf(stderr, "Failed to allocate memory for FileNode\n");
    return;
  }
  node->file_path = strdup(file_path);
  if (node->file_path == NULL) {
    fprintf(stderr, "Failed to duplicate file path\n");
    free(node);
    return;
  }
  node->next = NULL;

  pthread_mutex_lock(&queue->mutex);
  if (queue->head == NULL) { // empty
    queue->head = queue->tail = node;
  } else {
    queue->tail->next = node;
    queue->tail       = node;
  }
  pthread_cond_signal(&queue->cond);
  pthread_mutex_unlock(&queue->mutex);
}

char *file_queue_pop(FileQueue *queue) {
  pthread_mutex_lock(&queue->mutex);
  while (queue->head == NULL && !thread_manager->stop) {
    pthread_cond_wait(&queue->cond, &queue->mutex); // wait for file_queue_push to signal cond
  }
  if (thread_manager->stop && queue->head == NULL) { // TODO isto é inutil (se calhar)
    pthread_mutex_unlock(&queue->mutex);
    return NULL;
  }
  FileNode *node = queue->head;
  queue->head    = node->next;
  if (queue->head == NULL) {
    queue->tail = NULL;
  }
  pthread_mutex_unlock(&queue->mutex);

  char *file_path = node->file_path;
  free(node);
  return file_path;
}

void *process_file(void *arg) {
  (void)arg; // Unused
  while (1) {
    char *file_path = file_queue_pop(&thread_manager->queue);
    if (file_path == NULL) {
      break; // no more files to process
    }

    printf("opening file: %s\n", file_path); // TODO tirar

    int in_fd = open(file_path, O_RDONLY);
    if (in_fd < 0) {
      fprintf(stderr, "Failed to open job file: %s\n", file_path);
      free(file_path);
      continue;
    }
    // Create correspondent .out file;
    char out_path[PATH_MAX] = "";
    strncpy(out_path, file_path, strlen(file_path) - 4); // remove ".job"
    strcat(out_path, ".out");

    // TODO se calhar criar uma macro no constants.h para estas duas
    int open_flags = O_CREAT | O_WRONLY | O_TRUNC;
    // rw-rw-rw (or 0666)
    mode_t file_perms = S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH;

    int out_fd = open(out_path, open_flags, file_perms);
    if (out_fd < 0) {
      fprintf(stderr, "Failed to open .out file:%s\n", out_path);
      close(in_fd);
      free(file_path);
      continue;
    }

    read_file(in_fd, out_fd, file_path);

    close(in_fd);
    close(out_fd);
    free(file_path);
  }
  return NULL;
}

int thread_manager_init(size_t max_threads) {
  if (thread_manager != NULL) {
    fprintf(stderr, "Thread manager has already been initialized\n");
    return 1;
  }
  // TODO talvez retirar as verificações do malloc
  thread_manager = (t_manager *)malloc(sizeof(t_manager));
  if (thread_manager == NULL) {
    fprintf(stderr, "Failed to allocate memory for thread manager\n"); // TODO se calhar tirar estas mensagens de erro
    return 1;
  }

  file_queue_init(&thread_manager->queue);
  thread_manager->max_threads = max_threads;
  thread_manager->threads     = (pthread_t *)malloc(max_threads * sizeof(pthread_t));
  thread_manager->stop        = 0; // TODO se calhar tirar o stop

  for (size_t i = 0; i < max_threads; i++) {
    if (pthread_create(&thread_manager->threads[i], NULL, process_file, NULL) != 0) {
      fprintf(stderr, "Failed to create thread\n");
      return 1;
    }
  }

  return 0;
}

void thread_manager_add_job(const char *file_path) {
  file_queue_push(&thread_manager->queue, file_path);
}

void thread_manager_destroy() {

  pthread_mutex_lock(&thread_manager->queue.mutex);
  thread_manager->stop = 1;
  pthread_cond_broadcast(&thread_manager->queue.cond);
  pthread_mutex_unlock(&thread_manager->queue.mutex);

  for (size_t i = 0; i < thread_manager->max_threads; i++) {
    pthread_join(thread_manager->threads[i], NULL);
  }
  file_queue_destroy(&thread_manager->queue);
  free(thread_manager->threads);
  free(thread_manager);
}
