#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>
#include <errno.h>
#include "queue.h"
#include "acsh.h"

// PIDs
Queue *PIDs;
// Groups
Queue *Groups;

// Variável global indicando se a asch está ativa
int active = 0;


// Tratamento dos comandos, removendo caracteres indesejáveis
static int parse(char *line, char **argv, int nmax) {
    int i = 0;
    while(line && *line != '\0' && ++i <= nmax) { // se não está no fim de line...
        while(*line == ' ' || *line == '\t' || *line == '\n')
                *line++ = '\0'; // altera espaços em branco por \0
        *argv++ = line; // salva em argv
        while(*line != '\0' && *line != ' ' && 
             *line != '\t' && *line != '\n') 
            line++; // caminha sobre line enquanto...
    }
    *argv = '\0'; // marca o fim de argv
    return --i;
}

int turn_on() {
    active = 1;
    printf("\e[H\e[2J"); // limpar tela
    return 1;
}

int turn_off() {
    active = 0;
    return 1;
}

int is_active() {
    return active != 0;
}

void wait_command(char *cmd) {
    fputs("acsh> ", stdout);
    fgets(cmd, MAX_BUF, stdin); 
}

int get_commands(char *cmd, char **commands) {
    int i = -1;
    char* token = strtok(cmd, SEP_COMMANDS);
    while(++i <= MAX_COMMANDS+2 && token != NULL){
        commands[i] = (char*) malloc((strlen(token)+1)*sizeof(char));
        strcpy(commands[i], token);
        token = strtok(NULL, SEP_COMMANDS);
    }
    return i;
}

// Separar parametros a partir de um comando
// Note que o último parâmetro será NULL
static int get_params(char *command, char **params) {
    int n = parse(command, params, MAX_COMMAND_PARAM+3);
    params[n] = NULL; // último parametro é nulo, de acordo com a função execvp
    return n;
}

// Comandos internos
// Comparação com o primeiro comando recebido
static int internal_treatment(char *command, char **params) {
    if(strcmp(params[0], "cd") == 0) {
        if(chdir(params[1]) < 0)
            perror("*** ERROR: cd failed\n");
    }
    else if(strcmp(command, "exit") == 0) // finalizar os outros processos
        turn_off();
    else
        return 0;
    return 1;
}

// Execução de um comando em foreground
void execute_foreground(char *command, char **params) {
    pid_t pid;
    int status;
    if((pid = fork()) < 0) // cria um processo filho
        perror("*** ERROR: forking child process failed\n");
    else if(pid == 0) { // para os processos filhos
        if(execvp(*params, params) < 0) // executa o comando
            perror("*** ERROR: exec failed\n");
    }
    else
        while(wait(&status) != pid); // aguarda o comando ser completo
}

// Execução de um comando em background
void execute_background(char *command, char **params, pid_t id_group) {

}

// Execução de uma linha de comando
void execute_cmd(char *cmd) {
    size_t i, j;
    int m, n;
    char *commands[MAX_COMMANDS];
    char *params[MAX_COMMAND_PARAM+2];

    n = get_commands(cmd, commands); // separação dos comandos
    for(i = 0; i < n; i++) {
        m = get_params(commands[i], params); // separação dos parâmetros dos comandos
    
        if(internal_treatment(commands[i], params)) { // cd ou exit?
            free(commands[i]);
            return;
        }
        if(*params[m-1] == '%') { // foreground
            params[m-1] = NULL;
            execute_foreground(commands[i], params);
        }
        else {

        }
        free(commands[i]);
    }
}
