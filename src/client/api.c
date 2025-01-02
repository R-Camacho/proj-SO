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

  if (open_pipe(req_pipe_path, PIPE_PERMISSIONS) == -1) return 1;

  if (open_pipe(resp_pipe_path, PIPE_PERMISSIONS) == -1) return 1;

  if (open_pipe(notif_pipe_path, PIPE_PERMISSIONS) == -1) return 1;

  int server_fd;
  if ((server_fd = open_file(server_pipe_path, O_WRONLY)) == -1) return 1;

  char msg[1 + 3 * MAX_PIPE_PATH_LENGTH] = { 0 };

  msg[0] = OP_CODE_CONNECT;
  strncpy(msg + 1, req_pipe_path, MAX_PIPE_PATH_LENGTH);
  strncpy(msg + 1 + MAX_PIPE_PATH_LENGTH, resp_pipe_path, MAX_PIPE_PATH_LENGTH);
  strncpy(msg + 1 + 2 * MAX_PIPE_PATH_LENGTH, notif_pipe_path, MAX_PIPE_PATH_LENGTH);

  write_all(server_fd, msg, 1 + 3 * MAX_PIPE_PATH_LENGTH);
  close(server_fd);

  req_pipe_fd   = open_file(req_pipe_path, O_WRONLY);
  resp_pipe_fd  = open_file(resp_pipe_path, O_RDONLY);
  notif_pipe_fd = open_file(notif_pipe_path, O_RDONLY);
  *notif_pipe   = notif_pipe_fd;

  if (req_pipe_fd == -1 || resp_pipe_fd == -1 || notif_pipe_fd == -1) return 1;

  // TODO se calhar ler a resposta de maneira diferente
  char response[2] = { 0 };
  if (read_string(resp_pipe_fd, response) != 2) {
    fprintf(stderr, "Failed to read response from server\n");
    return 1;
  }

  // TODO se calhar printar noutro sitio???
  fprintf(stdout, "Server returned %d for operation: connect\n", response[1]);

  return 0;
}

int kvs_disconnect(void) {
  // TODO close pipes and unlink pipe files
  char msg = OP_CODE_DISCONNECT;
  if (write_all(req_pipe_fd, &msg, 1) == -1) return 1;

  if (close_file(req_pipe_fd) == -1) return 1;
  if (close_file(resp_pipe_fd) == -1) return 1;
  if (close_file(notif_pipe_fd) == -1) return 1;

  if (unlink_pipe(req_pipe_p) == -1) return 1;
  if (unlink_pipe(resp_pipe_p) == -1) return 1;
  if (unlink_pipe(notif_pipe_p) == -1) return 1;

  // TODO se calhar ler a resposta de maneira diferente
  char response[2] = { 0 };
  if (read_string(resp_pipe_fd, response) != 2) {
    fprintf(stderr, "Failed to read response from server\n");
    return 1;
  }

  // TODO se calhar printar noutro sitio???
  fprintf(stdout, "Server returned %d for operation: disconnect\n", response[1]);
  return 0;
}

int kvs_subscribe(const char *key) {
  // send subscribe message to request pipe and wait for response in response pipe
  char msg[1 + MAX_STRING_SIZE + 1] = { 0 };

  msg[0] = OP_CODE_SUBSCRIBE;
  strncpy(msg + 1, key, MAX_STRING_SIZE);
  if (write_all(req_pipe_fd, msg, 1 + MAX_STRING_SIZE + 1) == -1) return 1;

  char response[2] = { 0 };
  if (read_string(resp_pipe_fd, response) != 2) {
    fprintf(stderr, "Failed to read response from server\n");
    return 1;
  }

  // TODO se calhar printar noutro sitio???
  fprintf(stdout, "Server returned %d for operation: subscribe\n", response[1]);

  return 0;
}

int kvs_unsubscribe(const char *key) {
  // send unsubscribe message to request pipe and wait for response in response pipe
  char msg[1 + MAX_STRING_SIZE + 1] = { 0 };

  msg[0] = OP_CODE_UNSUBSCRIBE;
  strncpy(msg + 1, key, MAX_STRING_SIZE);
  if (write_all(req_pipe_fd, msg, 1 + MAX_STRING_SIZE + 1) == -1) return 1;

  char response[2] = { 0 };
  if (read_string(resp_pipe_fd, response) != 2) {
    fprintf(stderr, "Failed to read response from server\n");
    return 1;
  }

  // TODO se calhar printar noutro sitio???
  fprintf(stdout, "Server returned %d for operation: unsubscribe\n", response[1]);
  return 0;
}
