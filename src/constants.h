#ifndef CONSTANTS_H
#define CONSTANTS_H

#include <stddef.h>

#define MAX_WRITE_SIZE 256
#define MAX_STRING_SIZE 40
#define MAX_JOB_FILE_NAME_SIZE 256

extern unsigned int MAX_BACKUPS; // argv[2]
extern size_t MAX_THREADS;       // argv[3]

#endif // CONSTANTS_H