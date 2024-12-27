#ifndef COMMON_IO_H
#define COMMON_IO_H

#include <fcntl.h>
#include <stddef.h>
#include <sys/stat.h>
#include <sys/types.h>

/// Reads a given number of bytes from a file descriptor. Will block until all
/// bytes are read, or fail if not all bytes could be read.
/// @param fd File descriptor to read from.
/// @param buffer Buffer to read into.
/// @param size Number of bytes to read.
/// @param intr Pointer to a variable that will be set to 1 if the read was interrupted.
/// @return On success, returns 1, on end of file, returns 0, on error, returns -1
int read_all(int fd, void *buffer, size_t size, int *intr);

int read_string(int fd, char *str);

/// Writes a given number of bytes to a file descriptor. Will block until all
/// bytes are written, or fail if not all bytes could be written.
/// @param fd File descriptor to write to.
/// @param buffer Buffer to write from.
/// @param size Number of bytes to write.
/// @return On success, returns 1, on error, returns -1
int write_all(int fd, const void *buffer, size_t size);

void delay(unsigned int time_ms);

/// Opens a pipe with a given path and mode.
/// @param path Path to the pipe.
/// @param mode Mode to open the pipe with.
/// @return On success, returns 0, on error, returns -1
int open_pipe(const char *path, mode_t mode);

/// Opens a file with a given path and flags.
/// @param path Path to the file.
/// @param flags Flags to open the file with.
/// @return On success, returns the file descriptor, on error, returns -1
int open_file(const char *path, int flags);

#endif // COMMON_IO_H
