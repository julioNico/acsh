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

// Variável global indicando se a asch está ativa
int active = 0;

// Tratamento dos Sinal Ctrl+C
void end_handler_C(int signal)
{ 
    printf("\nNão adianta me enviar o sinal por Ctrl-C. Estou vacinado!!\n");   
}

// Tratamento dos Sinal Ctrl+Barra
void end_handler_B(int signal)
{
    printf("\nNão adianta me enviar o sinal por Ctrl-\\. Estou vacinado!!\n");
}

// Tratamento dos Sinal Ctrl+Z
void end_handler_Z(int signal)
{
    printf("\nNão adianta me enviar o sinal por Ctrl-Z. Estou vacinado!!\n");
}

// Tratamento dos comandos, removendo caracteres indesejáveis
static int parse(char *line, char **argv, int nmax)
{
    int i = 0;
    while (line && *line != '\0' && ++i <= nmax)
    { // se não está no fim de line...
        while (*line == ' ' || *line == '\t' || *line == '\n')
            *line++ = '\0'; // altera espaços em branco por \0
        *argv++ = line;     // salva em argv
        while (*line != '\0' && *line != ' ' &&
               *line != '\t' && *line != '\n')
            line++; // caminha sobre line enquanto...
    }
    *argv = '\0'; // marca o fim de argv
    return --i;
}

int turn_on()
{
    PIDs = Queue_init(100);
    active = 1;
    printf("\e[H\e[2J"); // limpar tela
    return 1;
}

int turn_off()
{
    active = 0;
    Queue_del(PIDs);
    return 1;
}

int is_active()
{
    return active != 0;
}

void wait_command(char *cmd)
{
    
    fputs("acsh> ", stdout);
    fgets(cmd, MAX_BUF, stdin);
}

int get_commands(char *cmd, char **commands)
{
    int i = -1;
    char *token = strtok(cmd, SEP_COMMANDS);
    while (++i <= MAX_COMMANDS + 2 && token != NULL)
    {
        commands[i] = (char *)malloc((strlen(token) + 1) * sizeof(char));
        strcpy(commands[i], token);
        token = strtok(NULL, SEP_COMMANDS);
    }
    return i;
}

// Separar parametros a partir de um comando
// Note que o último parâmetro será NULL
static int get_params(char *command, char **params)
{
    if (!command)
        return 0;
    int n = parse(command, params, MAX_COMMAND_PARAM + 3);
    params[n] = NULL; // último parametro é nulo, de acordo com a função execvp
    return n;
}

// Comandos internos
// Comparação com o primeiro comando recebido
static int internal_treatment(char **params)
{
    int pid;
    if (!params[0])
        return 0;

    if (strcmp(params[0], "cd") == 0)
    {
        if (chdir(params[1]) < 0)
            perror("*** ERROR: cd failed\n");
    }
    else if (strcmp(params[0], "exit") == 0)
    {
        while (!Queue_empty(PIDs)) { // finalizar os outros processos
            pid = dequeue(PIDs);
            killpg(pid, SIGKILL);
        }
        turn_off();
    }
    else
        return 0;
    return 1;
}

// Execução de uma linha de comando
void execute_cmd(char *cmd)
{
    size_t i = 0;
    pid_t pid, p_controle;
    int m, n, status;
    char *commands[MAX_COMMANDS];
    char *params[MAX_COMMAND_PARAM + 2];

    n = get_commands(cmd, commands); // separação dos comandos
    m = get_params(commands[0], params); // separação dos parâmetros do primeiro comando

    // Internal commands
    if (n == 1 && internal_treatment(params))
    {
        free(commands[0]);
        return;
    }

    // Foreground
    if (n == 1 && *params[m - 1] == '%')
    {
        params[m - 1] = NULL;
        signal(SIGINT, SIG_IGN);
        signal(SIGQUIT, SIG_IGN);
        signal(SIGTSTP, SIG_IGN);
        if ((pid = fork()) < 0)
            perror("*** ERROR: forking child process failed\n");
        else if (pid == 0)
        {
            if (execvp(*params, params) < 0)
                perror("*** ERROR: exec failed\n");
        }
        else {
            wait(NULL);
            free(commands[0]);
            return;
        }
    }

    // Background único
    if(n == 1) {
        if ((pid = fork()) < 0)
            perror("*** ERROR: forking child process failed\n");
        else if (pid == 0)
        {
            signal(SIGUSR1, SIG_IGN); // ignorando o SIGUSR1
            setsid(); // altera a sessão
            if (execvp(*params, params) < 0)
                perror("*** ERROR: exec failed\n");
        }
        else
            enqueue(PIDs, pid);
        return;
    }

    // Múltiplos processos em background
    if ((p_controle = fork()) < 0)
        perror("*** ERROR: forking child process failed\n");
    else if (p_controle != 0) {
        enqueue(PIDs, p_controle);
    }
    else if (p_controle == 0)
    {
        setsid(); // altera a sessão
        sinais(); // tratamento dos sinais
        for (i = 0; i < n; i++)
        {
            if(i != 0) m = get_params(commands[i], params);
            if ((pid = fork()) < 0) // cria um processo filho
                perror("*** ERROR: forking child process failed\n");
            else if (pid == 0)
            {
                signal(SIGUSR1, SIG_DFL); // tratamento default do SIGUSR1
                if (execvp(*params, params) < 0)
                    perror("*** ERROR: exec failed\n");
            }
        }
        while (wait(&status) != -1) { // tratamento do sinal SIGUSR1 para matar processos aglomerados
            if(WIFSIGNALED(status) && WTERMSIG(status) == SIGUSR1) { // algum filho recebeu SIGUSR1
                killpg(0, SIGKILL); // vai matar todos os processos com o mesmo grupo do filho
                // Send SIG to all processes in process group PGRP.
                // If PGRP is zero, send SIG to all processes in the current process's process group.
            }
        }
    }
}

// Função tratador de sinais.
int sinais()
{
    if ((signal(SIGINT, end_handler_C) == SIG_ERR) || (signal(SIGQUIT, end_handler_B) == SIG_ERR) || (signal(SIGTSTP, end_handler_Z) == SIG_ERR))
    {
        printf("Error while setting a signal handler\n");
        exit(EXIT_FAILURE);
    }
    return 0;
}