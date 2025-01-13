#include "io.h"

int read_all(int fd, void *buffer, size_t size, int *intr) {
  if (intr != NULL && *intr) {
    return -1;
  }
  size_t bytes_read = 0;
  while (bytes_read < size) {
    ssize_t result = read(fd, buffer + bytes_read, size - bytes_read);
    if (result == -1) {
      if (errno == EINTR) {
        if (intr != NULL) {
          *intr = 1;
          if (bytes_read == 0) {
            return -1;
          }
        }
        continue;
      }
      return -1;
    } else if (result == 0) {
      return 0;
    }
    bytes_read += (size_t)result;
  }
  return 1;
}

int read_string(int fd, char *str) {
  ssize_t bytes_read = 0;
  char ch;
  while (bytes_read < MAX_STRING_SIZE - 1) {
    if (read(fd, &ch, 1) != 1) {
      return -1;
    }
    if (ch == '\0' || ch == '\n') {
      break;
    }
    str[bytes_read++] = ch;
  }
  str[bytes_read] = '\0';
  return (int)bytes_read;
}

int write_all(int fd, const void *buffer, size_t size) {
  size_t bytes_written = 0;
  while (bytes_written < size) {
    ssize_t result = write(fd, buffer + bytes_written, size - bytes_written);
    if (result == -1) {
      if (errno == EINTR) {
        // error for broken PIPE (error associated with writting to the closed PIPE)
        continue;
      }
      perror("Failed to write to pipe");
      return -1;
    }
    bytes_written += (size_t)result;
  }
  return 1;
}

static struct timespec delay_to_timespec(unsigned int delay_ms) {
  return (struct timespec){ delay_ms / 1000, (delay_ms % 1000) * 1000000 };
}

void delay(unsigned int time_ms) {
  struct timespec delay = delay_to_timespec(time_ms);
  nanosleep(&delay, NULL);
}

int unlink_pipe(const char *path) {
  if (unlink(path) != 0 && errno != ENOENT) {
    fprintf(stderr, "Failed to unlink %s: %s\n", path, strerror(errno));
    return -1;
  }
  return 0;
}

int create_pipe(const char *path, mode_t mode) {
  // Remove pipe if exists
  if (unlink_pipe(path) != 0) {
    return -1;
  }

  if (mkfifo(path, mode) != 0) {
    fprintf(stderr, "Failed to open pipe: %s\n", strerror(errno));
    return -1;
  }

  fprintf(stdout, "[INFO]: Pipe %s created\n", path);
  return 0;
}

int open_file(const char *path, int flags) {
  int fd = open(path, flags);
  if (fd < 0) {
    fprintf(stderr, "Failed to open %s: %s\n", path, strerror(errno));
    return -1;
  }
  printf("[INFO]: File %s opened\n", path);
  return fd;
}

int close_file(int fd) {
  if (close(fd) != 0) {
    fprintf(stderr, "Failed to close: %s\n", strerror(errno));
    return -1;
  }
  return 0;
}
