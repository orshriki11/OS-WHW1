#include <iostream>
#include <signal.h>
#include "signals.h"
#include "Commands.h"

using namespace std;

void ctrlCHandler(int sig_num) {
    SmallShell &smash = SmallShell::getInstance();
    cout << "smash: got ctrl-C" << endl;
    if(smash.currentProcess != -1) {
        int PID = smash.currentProcess;
        smash.currentProcess = -1;
        smash.currentCmd = "";
        kill(PID, SIGINT);
        smash.sigStop = true;
        cout << "smash: process " << PID << " was killed" << endl;
    } else if(smash.watchProcess != -1){
        int PID = smash.watchProcess;
        smash.watchProcess = -1;
        smash.currentCmd = "";
        cout << "watchpro " << PID << endl;
        kill(PID, SIGINT);
        smash.sigStop = true;
        cout << "smash: process " << PID << " was killed" << endl;
    }
}
