#ifndef COMMON_IO_H
#define COMMON_IO_H

#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#include "src/common/constants.h"

/// Reads a given number of bytes from a file descriptor. Will block until all
/// bytes are read, or fail if not all bytes could be read.
/// @param fd File descriptor to read from.
/// @param buffer Buffer to read into.
/// @param size Number of bytes to read.
/// @param intr Pointer to a variable that will be set to 1 if the read was interrupted.
/// @return On success, returns 1, on end of file, returns 0, on error, returns -1
int read_all(int fd, void *buffer, size_t size, int *intr);

/// Reads a string from a file descriptor. Maximum string size is defined by
/// MAX_STRING_SIZE. Will fail if the string is too long.
/// @param fd File descriptor to read from.
/// @param str Buffer to read into.
/// @return On success, returns the number of bytes read, on error, returns -1
int read_string(int fd, char *str);

/// Writes a given number of bytes to a file descriptor. Will block until all
/// bytes are written, or fail if not all bytes could be written.
/// @param fd File descriptor to write to.
/// @param buffer Buffer to write from.
/// @param size Number of bytes to write.
/// @return On success, returns 1, on error, returns -1
int write_all(int fd, const void *buffer, size_t size);

/// Converts a delay in milliseconds to a timespec struct.
/// @param delay_ms Delay in milliseconds.
/// @return Timespec struct representing the delay.
static struct timespec delay_to_timespec(unsigned int delay_ms);

/// Sleeps for a given amount of time in milliseconds.
/// @param time_ms Time to sleep in milliseconds.
void delay(unsigned int time_ms);

/// Unlinks a pipe with a given path.
/// @param pathname Path to the pipe.
/// @return On success, returns 0, on error, returns -1
int unlink_pipe(const char *pathname);

/// Opens a pipe with a given path and mode.
/// @param path Path to the pipe.
/// @param mode Mode to open the pipe with.
/// @return On success, returns 0, on error, returns -1
int create_pipe(const char *path, mode_t mode);

/// Opens a file with a given path and flags.
/// @param path Path to the file.
/// @param flags Flags to open the file with.
/// @return On success, returns the file descriptor, on error, returns -1
int open_file(const char *path, int flags);

/// Closes a file descriptor.
/// @param fd File descriptor to close.
/// @return On success, returns 0, on error, returns -1
int close_file(int fd);

#endif // COMMON_IO_H
