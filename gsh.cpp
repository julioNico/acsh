#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <errno.h>
#include "list.h"

Lista* pids, *grupos;
int viva = 1;

/*      Funcoes auxiliares para a leitura do(s) comando(s)      */
void getComandos(char* cmd, char** comandos){ //Separa a entrada em comandos
    int i = 0;
    char* token = strtok(cmd, "#");
    while(token != NULL){
        token[strlen(token)-1] = '\0';

        comandos[i] = (char*) malloc((strlen(token)+1)*sizeof(char));
        strcpy(comandos[i], token);

        token = strtok(NULL, "#");
        i++;
    }
}

void getParams(char* cmd, char* params[5]){ //Separa os parametros do comando
    int i = 0;
    char* token = strtok(cmd, " ");
    while(token != NULL){
        params[i] = (char*) malloc((strlen(token)+1)*sizeof(char));

        strcpy(params[i], token);
        token = strtok(NULL, " ");
        i++;
    }
}

/*    Funcoes auxiliares para a execucao dos comandos   */
void executaCmd(char* comando, char* param[5]){
    getParams(comando, param); //Pega os parametros
    if(execvp(param[0], param) == -1) exit(1); //executa comando com a lista de parametros
}

pid_t criaProcBackG(int idGroup, char* comando){ //Cria processos em background
    char* param[5] = {NULL};
    pid_t pid = fork();
    if(pid < 0) perror("Failed to fork");
    else{
        if(pid == 0){
            signal(SIGINT, SIG_IGN); //Processos filhos ignoram sigint
            if(idGroup < 0) idGroup = 0; //Se o idGroup <0 entao eh o lider, o pgid sera o seu pid

            if(setpgid(0, idGroup) == -1) perror("Failed to set group"); // Seta o pgid para pertencer ao grupo dado
            else{
                if(rand() % 2)fork(); //Cria ghost
                executaCmd(comando, param);
            }
        }
    }
    pids = insereLista(pid, pids); //Guarda o pid do processo criado
    return pid;
}


/*     Ghost Shell     */
int main(){
    pids = criaLista(); //Cria lista de id dos processos
    grupos = criaLista(); //Cria lista de grupos

    //Muda o tratamento do sinal SIGTSTP
    signal(SIGTSTP, trataSIGTSTP);

    //Muda o tratamento do sinal SIGCHLD, 
    //que manipula lista de grupos (tratamento do sigint e sigtstp podem pegar a estrutura inconsistente)
    struct sigaction act2;
    act2.sa_handler = trataSIGCHLD; //novo handler
    act2.sa_flags = 0;
    if((sigemptyset(&act2.sa_mask) == -1) || (sigaddset(&act2.sa_mask, SIGINT) == -1)//bloqueia sigint e sigtstp
        || (sigaddset(&act2.sa_mask, SIGTSTP) || sigaction(SIGCHLD, &act2, NULL) == -1))
        perror("Failed to set SIGCHLD handler");

    /* Durante o tratamento do SIGINT todos os sinais sao bloqueados */
    struct sigaction act;
    act.sa_handler = trataSIGINT; //novo handler
    act.sa_flags = 0;

    //seta mascara de bits para bloquear todos os sinais e faz o sigaction
    if((sigfillset(&act.sa_mask) == -1) || (sigaction(SIGINT, &act, NULL) == -1))
        perror("Failed to set SIGINT to handle Ctrl-C");

    printf("\e[H\e[2J"); //Limpar tela
    while(viva){
        char cmd[100];
        fputs("gsh> ", stdout);
        fgets(cmd, 100, stdin); //Le comando(s)  
    
        char* comandos[6] = {NULL};
        getComandos(cmd, comandos); //Interpreta entrada em comandos separados

        //Verificar se ha comando interno
        if(!strcmp(comandos[0], "clean&die"))
            cleanANDdie();
        else if(!strcmp(comandos[0], "mywait")){
            mywait();
        }else{
            //Se for 1 comando, cria processo foreground
            if(comandos[1] == NULL){
                pid_t p = fork();
                if(p<0) perror("Failed to fork.");
                else{
                    if(p == 0){
                        signal(SIGINT, SIG_IGN); //Processos filhos ignoram sigint
                        if(rand() % 2)fork(); //Cria ghost
                        
                        char* param[5] = {NULL};
                        executaCmd(comandos[0], param);
                    }
                    pids = insereLista(p, pids);
                }
                waitpid(p, NULL, WUNTRACED); //espera processo foreground retornar/morrer
                
            }else{ //Mais de um comando, cria processos background em um unico grupo
                pid_t gid = criaProcBackG(-1, comandos[0]); //Cria lider do grupo, o seu id sera o id do grupo
                grupos = insereLista(gid, grupos); //insere grupo na lista de grupos

                int i =1;
                while(comandos[i] != NULL)
                    criaProcBackG(gid, comandos[i++]); //Cria outros processos no novo grupo
            }
        }
    }
    return 0;
}