#include "api.h"


int req_pipe_fd;
int resp_pipe_fd;
int notif_pipe_fd;
const char *req_pipe_p;
const char *resp_pipe_p;
const char *notif_pipe_p;
int session_id;

int kvs_connect(char const *req_pipe_path, char const *resp_pipe_path, char const *server_pipe_path, char const *notif_pipe_path, int *notif_pipe) {
  // TODO create pipes and connect; Falta ver o notif pipe
  req_pipe_p   = req_pipe_path;
  resp_pipe_p  = resp_pipe_path;
  notif_pipe_p = notif_pipe_path;
  // TODO debug print tirar
  printf("req_pipe_p: %s\n", req_pipe_p);
  printf("resp_pipe_p: %s\n", resp_pipe_p);

  if (create_pipe(req_pipe_path, PIPE_PERMISSIONS) == -1) return 1;

  if (create_pipe(resp_pipe_path, PIPE_PERMISSIONS) == -1) return 1;

  if (create_pipe(notif_pipe_path, PIPE_PERMISSIONS) == -1) return 1;

  int server_fd;
  if ((server_fd = open(server_pipe_path, O_WRONLY)) == -1) return 1;

  char msg[1 + 3 * MAX_PIPE_PATH_LENGTH] = { 0 };

  msg[0] = OP_CODE_CONNECT;
  strncpy(msg + 1, req_pipe_path, MAX_PIPE_PATH_LENGTH);
  strncpy(msg + 1 + MAX_PIPE_PATH_LENGTH, resp_pipe_path, MAX_PIPE_PATH_LENGTH);
  strncpy(msg + 1 + 2 * MAX_PIPE_PATH_LENGTH, notif_pipe_path, MAX_PIPE_PATH_LENGTH);

  if (write_all(server_fd, msg, 1 + 3 * MAX_PIPE_PATH_LENGTH) == -1 || close_file(server_fd) == -1) return 1;


  req_pipe_fd   = open_file(req_pipe_path, O_WRONLY);
  resp_pipe_fd  = open_file(resp_pipe_path, O_RDONLY);
  notif_pipe_fd = open_file(notif_pipe_path, O_RDONLY);

  *notif_pipe = notif_pipe_fd;

  if (req_pipe_fd == -1 || resp_pipe_fd == -1 || notif_pipe_fd == -1) return 1;

  read_response("connect");
  return 0;
}

int kvs_disconnect(void) {
  // TODO close pipes and unlink pipe files
  char msg = OP_CODE_DISCONNECT;
  if (write_all(req_pipe_fd, &msg, 1) == -1) return 1;

  if (close_file(req_pipe_fd) == -1) return 1;
  if (close_file(notif_pipe_fd) == -1) return 1;

  if (unlink_pipe(req_pipe_p) == -1) return 1;
  if (unlink_pipe(notif_pipe_p) == -1) return 1;

  read_response("disconnect");
  if (close_file(resp_pipe_fd) == -1) return 1;
  if (unlink_pipe(resp_pipe_p) == -1) return 1;
  return 0;
}

int kvs_subscribe(const char *key) {
  // send subscribe message to request pipe and wait for response in response pipe
  char msg[1 + MAX_STRING_SIZE + 1] = { 0 };

  msg[0] = OP_CODE_SUBSCRIBE;
  strncpy(msg + 1, key, MAX_STRING_SIZE);
  if (write_all(req_pipe_fd, msg, 1 + MAX_STRING_SIZE + 1) == -1) return 1;

  read_response("subscribe");
  return 0;
}

int kvs_unsubscribe(const char *key) {
  // send unsubscribe message to request pipe and wait for response in response pipe
  char msg[1 + MAX_STRING_SIZE + 1] = { 0 };

  msg[0] = OP_CODE_UNSUBSCRIBE;
  strncpy(msg + 1, key, MAX_STRING_SIZE);
  if (write_all(req_pipe_fd, msg, 1 + MAX_STRING_SIZE + 1) == -1) return 1;

  read_response("unsubscribe");
  return 0;
}

void read_response(char *operation) {
  char response[2] = { 0 };
  int intr         = 0;
  while (1) {
    ssize_t bytes_read = read_all(resp_pipe_fd, response, sizeof(response), &intr);
    if (bytes_read == -1) {
      if (errno == EAGAIN || errno == EWOULDBLOCK) {
        // No data to read
        continue;
      }
      if (intr) {
        // Thread was cancelled
        fprintf(stderr, "Read interrupted\n");
        return;
      }
      // Other error occurred
      fprintf(stderr, "Failed to read from response pipe: %s\n", strerror(errno));
      continue;
    }

    if (bytes_read == 0) {
      continue;
    }
    break;
  }

  fprintf(stdout, "Server returned %c for operation: %s\n", response[1], operation);
}