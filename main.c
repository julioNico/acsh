#include <stdio.h>
#include <signal.h>
#include "acsh.h"


int main( ) {
    char cmd[MAX_BUF];
    turn_on();

    while(is_active()) {
        wait_command(cmd); // leitura
        execute_cmd(cmd); // execução
    }
    return 0;
}