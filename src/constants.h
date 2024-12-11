#ifndef CONSTANTS_H
#define CONSTANTS_H

#include <pthread.h>

#define MAX_WRITE_SIZE 256
#define MAX_STRING_SIZE 40
#define MAX_JOB_FILE_NAME_SIZE 256

extern size_t MAX_BACKUPS; // argv[2]

extern pthread_mutex_t backup_mutex;
extern pthread_cond_t backup_cond;
extern size_t active_backups;

#endif // CONSTANTS_H