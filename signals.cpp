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
        cout << "smash: process " << PID << " was killed" << endl;
    }
}
