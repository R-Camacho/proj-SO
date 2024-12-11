#ifndef THREAD_H
#define THREAD_H

#define _DEFAULT_SOURCE // to use usleep

#include <fcntl.h>
#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "reader.h"

typedef struct file_node {
  char *file_path;
  struct file_node *next;
} FileNode;

typedef struct file_queue {
  FileNode *head;
  FileNode *tail;
  pthread_mutex_t mutex;
  pthread_cond_t cond;
} FileQueue;

typedef struct thread_manager {
  pthread_t *threads;
  size_t max_threads;
  FileQueue queue;
  int stop;
} t_manager;

extern t_manager *thread_manager;

// TODO verificar comments (ortografia / correcao)

/// Initializes the file queue.
/// @param queue Pointer to the FileQueue to be initialized.
/// @return 0 if the queue was initialized successfully, 1 otherwise.
int file_queue_init(FileQueue *queue);

/// Destroys the file queue and frees all associated resources.
/// @param queue Pointer to the FileQueue to be destroyed.
/// @return 0 if the queue was destroyed successfully, 1 otherwise.
int file_queue_destroy(FileQueue *queue);

/// Pushes a file path onto the file queue.
/// @param queue Pointer to the FileQueue where the file path will be pushed.
/// @param file_path The file path to be pushed onto the queue.
void file_queue_push(FileQueue *queue, const char *file_path);

/// Pops a file path from the file queue.
/// @param queue Pointer to the FileQueue from which the file path will be popped.
/// @return The file path popped from the queue, or NULL if the queue is empty or the thread manager is stopping.
char *file_queue_pop(FileQueue *queue);

/// Processes files by reading from the file queue and performing operations on them.
/// @param arg Unused argument.
/// @return NULL when the processing is complete.
void *process_file(void *arg);

/// Initializes the thread manager with a specified number of threads.
/// @param max_threads The maximum number of threads to be created.
/// @return 0 if the thread manager was initialized successfully, 1 otherwise.
int thread_manager_init(size_t max_threads);

/// Adds a job to the thread manager's file queue.
/// @param file_path The path of the job file to be added.
void thread_manager_add_job(const char *file_path);

/// Destroys the thread manager and frees all associated resources.
void thread_manager_destroy();

#endif // THREAD_H