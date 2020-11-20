#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>
#include <errno.h>
#include "queue.h"
#include "acsh.h"

#define READ 0
#define WRITE 1

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
    PIDs = Queue_init(100);
    active = 1;
    printf("\e[H\e[2J"); // limpar tela
    return 1;
}

int turn_off() {
    active = 0;
    Queue_del(PIDs);
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
static int internal_treatment(char **params) {
    if(strcmp(params[0], "cd") == 0) {
        if(chdir(params[1]) < 0)
            perror("*** ERROR: cd failed\n");
    }
    else if(strcmp(params[0], "exit") == 0) // finalizar os outros processos
        turn_off();
    else
        return 0;
    return 1;
}

// Execução de um comando em foreground
void execute_foreground(char **params) {
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
void execute_background(char **params, pid_t pid_group) {
    pid_t pid;
    if((pid = fork()) < 0) // cria um processo filho
        perror("*** ERROR: forking child process failed\n");
    else if(pid == 0) { // para os processos filhos
        if(setpgid(pid, pid_group) == -1) // seta o pgid para pertencer ao pid_group
            perror("*** ERROR: failed to set group");
        if(execvp(*params, params) < 0) // executa o comando
            perror("*** ERROR: exec failed\n");
    }
}

// Execução de uma linha de comando
void execute_cmd(char *cmd) {
    size_t i = 0;
    int m, n;
    int fd[2];
    char *commands[MAX_COMMANDS];
    char *params[MAX_COMMAND_PARAM+2];
    pid_t pid, gid;

    if(pipe(fd) < 0) // Failed to create a pipe
        perror("*** ERROR: failed to create a pipe");

    n = get_commands(cmd, commands); // separação dos comandos

    // First command
    m = get_params(commands[i], params);
    if(n == 1 && internal_treatment(params)) { // cd ou exit?
        free(commands[i]);
        return;
    }
    if(*params[m-1] == '%') { // foreground
        params[m-1] = NULL;
        execute_foreground(params);
    }
    else {
        if((pid = fork()) < 0) // cria um processo filho
            perror("*** ERROR: forking child process failed\n");
        else if(pid == 0) { // para os processos filhos
            if(setpgid(0, 0) == -1)
                perror("*** ERROR: failed to set group"); // seta o pgid para pertencer ao grupo dado

            // Send the pgdi by pipes
            gid = getpgid(pid);
            close(fd[READ]);
		    write(fd[WRITE], &gid, sizeof(pid_t));

            if(execvp(*params, params) < 0) // executa o comando
                perror("*** ERROR: exec failed\n");
            return;
        }
    }

    // Rest of commands
    for(i = 1; i < n; i++) {
        m = get_params(commands[i], params); // separação dos parâmetros dos comandos
        if(*params[m-1] == '%') { // foreground
            params[m-1] = NULL;
            execute_foreground(params);
            return;
        }
        else {
            close(fd[WRITE]);
            read(fd[READ], &gid, sizeof(pid_t)); // value of pgid group
            printf("%d\n", gid);
            execute_background(params, gid);
        }
        free(commands[i]);
    }
}
