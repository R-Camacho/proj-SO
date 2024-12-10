#include "thread_manager.h"

t_manager *thread_manager = NULL;

int thread_manager_init(size_t max_threads) {
  if (thread_manager != NULL) {
    fprintf(stderr, "Thread manager has already been initialized\n");
    return 1;
  }
  thread_manager = (t_manager *)malloc(sizeof(t_manager));
  if (thread_manager == NULL) {
    fprintf(stderr, "Failed to allocate memory for thread manager\n"); // TODO se calhar tirar estas mensagens de erro
    return 1;
  }

  thread_manager->max_threads = max_threads;
  thread_manager->threads     = (pthread_t *)malloc(max_threads * sizeof(pthread_t));


  return 0;
}


void thread_manager_create(void *(*start_routine)(void *), void *arg) {
}


void thread_manager_destroy() {
}
