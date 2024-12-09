#ifndef READER_H
#define READER_H

#include "stdio.h"

#include "operations.h"
#include "parser.h"

/// Reads a file and executes the commands in it.
/// @param in_fd File descriptor to read from.
/// @param out_fd File descriptor to write to.
void read_file(int in_fd, int out_fd);

#endif // READER_H