#include "api.h"
#include "src/common/constants.h"
#include "src/common/io.h"
#include "src/common/protocol.h"

int req_pipe_fd;
int resp_pipe_fd;
int notif_pipe_fd;
const char *req_pipe_p;
const char *resp_pipe_p;
const char *notif_pipe_p;
int session_id;

int kvs_connect(char const *req_pipe_path, char const *resp_pipe_path, char const *server_pipe_path, char const *notif_pipe_path, int *notif_pipe) {
  // TODO create pipes and connect
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

  if (req_pipe_fd == -1 || resp_pipe_fd == -1 || notif_pipe_fd == -1) return 1;

  // TODO se calhar ler a resposta de maneira diferente
  char *response = (char *)malloc((2 + 1) * sizeof(char));
  if (read_string(resp_pipe_fd, response) != 2) {
    fprintf(stderr, "Failed to read response from server\n");
    return 1;
  }

  // TODO se calhar printar noutro sitio???
  fprintf(stdout, "Server returned %d for operation: connect\n", response[1]);

  return 0;
}

int kvs_disconnect(void) {
  // close pipes and unlink pipe files
  return 0;
}

int kvs_subscribe(const char *key) {
  // send subscribe message to request pipe and wait for response in response pipe
  return 0;
}

int kvs_unsubscribe(const char *key) {
  // send unsubscribe message to request pipe and wait for response in response pipe
  return 0;
}
