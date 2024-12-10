#ifndef THREAD_H
#define THREAD_H

#include <pthread.h>
#include <stdio.h>

typedef struct thread_manager {
  pthread_t *threads;
  size_t max_threads;
} t_manager;

extern t_manager *thread_manager;

int thread_manager_init(size_t max_threads);

void thread_manager_create(void *(*start_routine)(void *), void *arg);

void thread_manager_destroy();


#endif // THREAD_H