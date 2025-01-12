#ifndef COMMON_CONSTANTS_H
#define COMMON_CONSTANTS_H

// constantes partilhadas entre cliente e servidor
#define MAX_SESSION_COUNT 8     // num max de sessoes no server, 1 default, necessario alterar mais tarde
#define STATE_ACCESS_DELAY_US   // delay a aplicar no server
#define MAX_PIPE_PATH_LENGTH 40 // tamanho max do caminho do pipe
#define MAX_REGISTER_LENGTH (1 + 3 * MAX_PIPE_PATH_LENGTH) // tamanho max da mensagem de registo
#define MAX_NOTIFICATION_SIZE \
  (1 + MAX_STRING_SIZE + 1 + 1 + MAX_STRING_SIZE + 1 + 1 + 1) // tamanho max da mensagem de notificacao
#define MAX_STRING_SIZE 40

#define MAX_WRITE_SIZE 256

#define MAX_NUMBER_SUB 10

#define PIPE_PERMISSIONS 0640 // permissoes dos pipes

#endif // COMMON_CONSTANTS_H