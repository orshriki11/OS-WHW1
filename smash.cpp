#include <iostream>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include "Commands.h"
#include "signals.h"

int main(int argc, char *argv[]) {
    if (signal(SIGINT, ctrlCHandler) == SIG_ERR) {
        perror("smash error: failed to set ctrl-C handler");
    }

    //TODO: setup sig alarm handler

    SmallShell &smash = SmallShell::getInstance();
    while (true) {
        //smash.smash_prompt
        smash.pipe = false;
        std::cout << smash.smash_prompt <<"> ";
        std::string cmd_line;
        std::getline(std::cin, cmd_line);
        smash.executeCommand(cmd_line.c_str());
        if(smash.pipe) {
            break;
        }
    }
    return 0;
}