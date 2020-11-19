#ifndef _ACSH_H_
#define _ACSH_H_

#include <sys/types.h>


#define MAX_BUF 100
#define MAX_COMMANDS 5
#define MAX_COMMAND_PARAM 3
#define SEP_COMMANDS "<3"


// Ativa a acsh
int turn_on();

// Desativa a asch
int turn_off();

// asch está ativa?
int is_active();

// Aguardar por um comando
// acsh> 
void wait_command(char*);

// Pegar comandos a partir da leitura do cmd
int get_commands(char*, char**);

// Execução de um comando em foreground
void execute_foreground(char*, char**);

// Execução de um comando em background
void execute_background(char*, char**, pid_t);

// Execução de uma linha de comando
void execute_cmd(char*);

// Execução de uma linha de comando
void execute_cmd(char*);

#endif