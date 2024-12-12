#ifndef READER_H
#define READER_H


#include <semaphore.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "constants.h"
#include "operations.h"
#include "parser.h"

/// Reads a file and executes the commands in it.
/// @param in_fd File descriptor to read from.
/// @param out_fd File descriptor to write to.
/// @param job_path Path to the job file.
void read_file(int in_fd, int out_fd, const char *job_path);

#endif // READER_H