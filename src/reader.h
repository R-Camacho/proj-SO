#ifndef READER_H
#define READER_H

#include "stdio.h"
#include "unistd.h"

#include "operations.h"
#include "parser.h"

int read_batch(int job_fd, int out_fd);

#endif // READER_H