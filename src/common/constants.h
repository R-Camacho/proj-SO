#ifndef COMMON_CONSTANTS_H
#define COMMON_CONSTANTS_H

// constantes partilhadas entre cliente e servidor
#define MAX_SESSION_COUNT 1     // num max de sessoes no server, 1 default, necessario alterar mais tarde
#define STATE_ACCESS_DELAY_US   // delay a aplicar no server
#define MAX_PIPE_PATH_LENGTH 40 // tamanho max do caminho do pipe
#define MAX_STRING_SIZE 40
#define MAX_NUMBER_SUB 10

#define PIPE_PERMISSIONS 0640 // permissoes dos pipes

#endif // COMMON_CONSTANTS_H