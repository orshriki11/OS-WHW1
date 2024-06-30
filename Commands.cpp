#include <unistd.h>
#include <string.h>
#include <iostream>
#include <vector>
#include <sstream>
#include <sys/wait.h>
#include <iomanip>
#include "Commands.h"
#include <regex>

using namespace std;

const std::string WHITESPACE = " \n\r\t\f\v";

#if 0
#define FUNC_ENTRY()  \
  cout << __PRETTY_FUNCTION__ << " --> " << endl;

#define FUNC_EXIT()  \
  cout << __PRETTY_FUNCTION__ << " <-- " << endl;
#else
#define FUNC_ENTRY()
#define FUNC_EXIT()
#endif



string _ltrim(const std::string &s) {
    size_t start = s.find_first_not_of(WHITESPACE);
    return (start == std::string::npos) ? "" : s.substr(start);
}

string _rtrim(const std::string &s) {
    size_t end = s.find_last_not_of(WHITESPACE);
    return (end == std::string::npos) ? "" : s.substr(0, end + 1);
}

string _trim(const std::string &s) {
    return _rtrim(_ltrim(s));
}

int _parseCommandLine(const char *cmd_line, char **args) {
    FUNC_ENTRY()
    int i = 0;
    std::istringstream iss(_trim(string(cmd_line)).c_str());
    for (std::string s; iss >> s;) {
        args[i] = (char *) malloc(s.length() + 1);
        memset(args[i], 0, s.length() + 1);
        strcpy(args[i], s.c_str());
        args[++i] = NULL;
    }
    return i;

    FUNC_EXIT()
}

bool _isBackgroundComamnd(const char *cmd_line) {
    const string str(cmd_line);
    return str[str.find_last_not_of(WHITESPACE)] == '&';
}

void _removeBackgroundSign(char *cmd_line) {
    const string str(cmd_line);
    // find last character other than spaces
    unsigned int idx = str.find_last_not_of(WHITESPACE);
    // if all characters are spaces then return
    if (idx == string::npos) {
        return;
    }
    // if the command line does not end with & then return
    if (cmd_line[idx] != '&') {
        return;
    }
    // replace the & (background sign) with space and then remove all tailing spaces.
    cmd_line[idx] = ' ';
    // truncate the command line string up to the last non-space character
    cmd_line[str.find_last_not_of(WHITESPACE, idx) + 1] = 0;
}

char** _initArgs(const char *cmd_line, int *argsCount)
{
    char** args = (char **) malloc(COMMAND_MAX_ARGS * sizeof(char **));
    if(!args)
        return nullptr;
    for(int i = 0; i < COMMAND_MAX_ARGS; i++)
        args[i] = nullptr;
    int argsC = _parseCommandLine(cmd_line, args);
    if(argsC == -1)
        args = nullptr;
    *argsCount = argsC;
    return args;
}

bool is_number(const std::string& s)
{
    std::string::const_iterator it = s.begin();
    while (it != s.end() && std::isdigit(*it)) ++it;
    return !s.empty() && it == s.end();
}

string _aliasName(const char *alias){
    unsigned last = string(alias).find('=');
    return string(alias).substr(0, last);

}

string _aliasCmd(const char *alias){
    unsigned last = string(alias).find_last_of('\'');
    unsigned first = string(alias).find_first_of('\'');
    return string(alias).substr(first, last);

}


// TODO: Add your implementation for classes in Commands.h

JobsList SmallShell::jobsList;
pid_t SmallShell::pid = getppid();

JobsList::JobEntry *JobsList::getJobById(int jobId) {
    for(auto& job : jobsList) {
        if(job.jobID == jobId)
            return &job;
    }
    return nullptr;
}

JobsList::JobEntry *JobsList::getLastJob(int *lastJobId) {
    int maxJobID = -1;
    for(auto& job : jobsList) {
        if(job.jobID > maxJobID)
            maxJobID = job.jobID;
    }
    *lastJobId = maxJobID;
    return getJobById(maxJobID);
}

void JobsList::removeJobById(int jobId) {
    for(auto it = jobsList.begin(); it != jobsList.end(); ++it) {
        auto job = *it;
        if(job.jobID == jobId) {
            auto jobToErase = job;
            jobsList.erase(it);
            break;
        }
    }
}

void JobsList::killAllJobs() {
    for(auto& job : jobsList) {
        cout << job.jobPID << ": " << job.command << endl;
        kill(job.jobPID, SIGKILL);
    }
}
void JobsList::printJobsList() {
    for(auto& job : jobsList) {
        cout << job.jobPID << ": " << job.command << endl;
    }
}

JobsList::JobEntry *JobsList::getLastStoppedJob(int *jobId) {
    int maxJobID = -1;
    for(auto& job : jobsList) {
        if(job.isStopped)
            maxJobID = job.jobID;
    }
    *jobId = maxJobID;
    return getJobById(maxJobID);
}

void JobsList::removeFinishedJobs() {
    if(jobsList.empty()) {
        maxJobID = 1;
        return;
    }
    SmallShell &smash = SmallShell::getInstance();

    for (auto it = jobsList.begin(); it != jobsList.end(); ++it) {
        if(!smash.isPipe) {
            auto job = *it;
            int pidStatus;
            int wait = waitpid(job.jobPID, &pidStatus, WNOHANG);
            if(wait == job.jobPID || wait == -1) {
                jobsList.erase(it);
                --it;
            }
        }
    }

    int maxID = 0;

    for(auto& job : jobsList) {
        if(job.jobID > maxID)
            maxID = job.jobID;
    }

    maxJobID = maxID + 1;

}
void JobsList::addJob(Command *cmd, bool isStopped) {
    removeFinishedJobs();
    std::string cmd_line(cmd->getCmd());
    SmallShell &smash = SmallShell::getInstance();
    if (CmdFg) {
        for(auto it = jobsList.begin(); it != jobsList.end(); ++it){
            auto job = *it;
            if(job.jobID > smash.fgJobID)
                break;
        }
        jobsList.insert(it, JobEntry(smash.fgJobID, pid, cmd_line, isStopped));
        return;
    }
    jobsList.push_back(JobEntry(smash.fgJobID, pid, cmd_line, isStopped));
    maxJobID++;
}





ForegroundCommand::ForegroundCommand(const char *cmd_line, JobsList *jobs) : BuiltInCommand(cmd_line),
jobs(jobs){};


void ForegroundCommand::execute() {
    int argsCount;
    char **args = _initArgs(this->cmd_line, &argsCount);
    int jobID;
    JobsList::JobEntry *job;
    SmallShell &smash = SmallShell::getInstance();
    if(argsCount == 2) {
        try
        {
            jobID = stoi(args[1]);
        }
        catch(const std::exception& e)
        {
            std::cerr << "smash error: fg: invalid arguments" << endl;
            return;
        }
        job = jobs->getJobById(jobID);
    }
    else if(argsCount == 1) {
        job = jobs->getLastJob(&jobID);
        if(!job){
            std::cerr << "smash error: fg: jobs list is empty" << endl;
            return;
        }
    }
    else {
        std::cerr << "smash error: fg: invalid arguments" << endl;
        return;
    }

    if(jobID >= 0 && job){
        int jobPID = job->jobPID;
        if(job->isStopped && kill(jobPID, SIGCONT) == SYS_FAIL)
        {
            perror("smash error: kill failed");
            return;
        }
        int pStatus;
        smash.currentProcess = jobPID;
        smash.currentCmd = job->command;
        smash.fgJobID = jobID;
        jobs->removeJobById(jobID);
        cout << job->command << " : " << job->jobPID << endl;
        if(waitpid(jobPID, &pStatus, WUNTRACED) == SYS_FAIL)
        {
            perror("smash error: waitpid failed");
            return;
        }

    } else {
        std::cerr << "smash error: fg: job-id " << jobID << " does not exist" << endl;
        return;
    }

    //look for job-id in background

    //print the cmd-line of that job + its PID
    //Then wait using waitpid
    //If ID specified - find the requested job
    //If ID not specified - maximal ID
    //!! After bringing job to FG remove from job list.

}

//Quit Command
QuitCommand::QuitCommand(const char *cmd_line, JobsList *jobs)  : BuiltInCommand(cmd_line) {}

void QuitCommand::execute() {
    int argsCount;
    char **args = _initArgs(this->cmd_line, &argsCount);
    SmallShell &smash = SmallShell::getInstance();

    if(argsCount >= 2 && string(args[1]).compare("kill") == 0) {
        cout << "smash: sending SIGKILL signal to " << smash.jobsList.jobsList.size() << " jobs:" << endl;
        smash.jobsList.killAllJobs();
    }
    exit(0);
    //the command will exit the smash
    //If kill arg is specified then smash should send SIGKILL all of unfinished
    //jobs before exiting
    //If kill was specified then print number of jobs killed, their PIDS and commandlines.
}


KillCommand::KillCommand(const char *cmd_line, JobsList *jobs)  : BuiltInCommand(cmd_line) {}

void KillCommand::execute() {
    int argsCount;
    char **args = _initArgs(this->cmd_line, &argsCount);
    SmallShell &smash = SmallShell::getInstance();

    if(argsCount != 3) {
        cerr << "smash error: kill: invalid arguments" << endl;
    } else {
        int sigNum;
        int jobID;
        try {
            if(!is_number(args[2]))
                throw exception();
            //char firstChar = string(args[1]).at(0);
            if (string(args[1]).at(0) != '-')
                throw exception();
            jobID = stoi(args[2]);
            if(!is_number(string(args[1]).erase(0, 1)))
                throw exception();
            sigNum = stoi(string(args[1]).erase(0, 1));
        } catch (exception &) {
            cerr << "smash error: kill: invalid arguments" << endl;
            return;
        }
        if (JobsList::JobEntry *job = smash.jobsList.getJobById(jobID)) {
            int jobPID = job->jobPID;

            if(kill(jobPID, sigNum) == SYS_FAIL) {
                perror("smash error: kill failed");
                return;
            }
            cout << "signal number " << sigNum << " was sent to pid " << jobPID << endl;
            if(sigNum == SIGTSTP) {
                job->isStopped = true;
            } else if (sigNum == SIGCONT) {
                job->isStopped = false;
            }

        } else {
            cerr << "smash error: kill: job-id " << jobID << " does not exist" << endl;
        }
    }
    //
}


aliasCommand::aliasCommand(const char *cmd_line)  : BuiltInCommand(cmd_line) {
}

void aliasCommand::execute() {
    int argsCount;
    char **args = _initArgs(this->cmd_line, &argsCount);
    SmallShell &smash = SmallShell::getInstance();
    regex alias_expr ("^alias [a-zA-Z0-9_]+='[^']*'$");

    if(argsCount == 1) {
        for(auto& alias : smash.aliasList) {
            cout << alias.aliasName << "='" << alias.commandLine << "'" << endl;
        }
    }
    if(argsCount == 2)
    {
        if(!regex_match(cmd_line,alias_expr))
        {
            cerr << "smash error: alias: invalid alias format" << endl;
            return;
        }
        string aliasName = _aliasName(args[1]);
        string aliasCmd = _aliasCmd(args[1]);
        for(auto& alias : smash.aliasList) {
            if (alias.aliasName.compare(aliasName)) {
                cerr << "smash error: alias: " << aliasName << " already exists or is a reserved command" << endl;
                return;
            }
        }
        if (std::find(smash.reservedCommands.begin(), smash.reservedCommands.end(), aliasName) !=
                        smash.reservedCommands.end())
        {
            cerr << "smash error: alias: " << aliasName << " already exists or is a reserved command" << endl;
            return;
        }
        const int length = aliasCmd.length();
        char* aliasCmdChars = new char[length + 1];
        strcpy(aliasCmdChars, aliasCmd.c_str());
        AliasEntry newAlias;
        newAlias.aliasName = aliasName;
        newAlias.commandLine = aliasCmdChars;
        smash.aliasList.push_back(newAlias);

    } else {

    }
}

unaliasCommand::unaliasCommand(const char *cmd_line)  : BuiltInCommand(cmd_line) {
}

void unaliasCommand::execute() {
    int argsCount;
    char **args = _initArgs(this->cmd_line, &argsCount);
    SmallShell &smash = SmallShell::getInstance();
    if(argsCount >= 2) {
        for(int i=1; i < COMMAND_MAX_ARGS; i++) {
            bool notFound = true;
            for (auto it = smash.aliasList.begin(); it != smash.aliasList.end(); ++it) {
                if(std::equal(it->aliasName, args[i])) {
                    auto alias = *it;
                    smash.aliasList.erase(it);
                    notFound = false;
                    break;
                }
            }
            if(notFound) {
                cerr << "smash error: unalias: not enough arguments" << endl;
                return;
            }
        }
    }
    else {
        cerr << "smash error: unalias: not enough arguments" << endl;
    }

}

ExternalCommand::ExternalCommand(const char *cmd_line) : Command(cmd_line) {}

void ExternalCommand::execute() {
    string trimCmd = _trim(string(cmd_line));
    char cmdCopy[COMMAND_MAX_LENGTH];
    strcpy(cmdCopy, trimCmd.c_str());
    bool isBg = _isBackgroundComamnd(cmd_line);
    if(isBg)
        _removeBackgroundSign(cmdCopy);


    char executable[] = "/bin/bash";
    char executable_name[] = "/bin/bash";
    char flag[] = "-c";
    char *fork_args[] = {executable_name, flag, cmdCopy, nullptr};
    pid_t pid = fork();

    if (pid == 0) {
        if (setpgrp() == SYS_FAIL) {
            perror("smash error: setpgrp failed");
            return;
        }
        if (execv(executable, fork_args) == SYS_FAIL) {
            perror("smash error: execv failed");
            return;
        }
    } else {
        SmallShell &smash = SmallShell::getInstance();
        if (isBg) {
            smash.jobsList.addJob(this, pid);
        } else {
            smash.currentProcess = pid;
            smash.currentCmd = cmd_line;
            int pstatus;
            if (waitpid(pid, &pstatus, WUNTRACED) == SYS_FAIL) {
                perror("smash error: waitpid failed");
                return;
            }
        }
    }


}


SmallShell::SmallShell() {
// TODO: add your implementation
}

SmallShell::~SmallShell() {
// TODO: add your implementation
}

/**
* Creates and returns a pointer to Command class which matches the given command line (cmd_line)
*/
Command *SmallShell::CreateCommand(const char *cmd_line) {

  string cmd_s = _trim(string(cmd_line));
  string firstArg = cmd_s.substr(0, cmd_s.find_first_of(" \n"));

  if(firstArg.compare("chprompt") == 0) {
      //return new ChpromptCommand(cmd_line);
  }
  else if (firstArg.compare("showpid") == 0) {
      return new ShowPidCommand(cmd_line);
  }
  else if (firstArg.compare("pwd") == 0) {
    return new GetCurrDirCommand(cmd_line);
  }
  //Need to add a proper last directory ref.
  else if (firstArg.compare("cd") == 0) {
      return new ChangeDirCommand(cmd_line, &currentDir);
  }
      //Need to add a proper last directory ref.
  else if (firstArg.compare("jobs") == 0) {
      return new JobsCommand(cmd_line);
  }
  else if (firstArg.compare("fg") == 0) {
      return new ForegroundCommand(cmd_line);
  }
  else if (firstArg.compare("quit") == 0) {
      return new QuitCommand(cmd_line);
  }
  else if (firstArg.compare("kill") == 0) {
      return new KillCommand(cmd_line);
  }
  else if (firstArg.compare("alias") == 0) {
      return new aliasCommand(cmd_line);
  }
  else if (firstArg.compare("unalias") == 0) {
      return new unaliasCommand(cmd_line);
  }
  else {
      //!!Need to check if command is alias and if so then combine args into it.
      char* alias;
      if(aliasCheck(cmd_line, &alias))
          return CreateCommand(alias);
      else
        return new ExternalCommand(cmd_line);

  }

    return nullptr;
}

void SmallShell::executeCommand(const char *cmd_line) {
    // TODO: Add your implementation here
    Command* cmd = CreateCommand(cmd_line);
    cmd->execute();

    // for example:
    // Command* cmd = CreateCommand(cmd_line);
    // cmd->execute();
    // Please note that you must fork smash process for some commands (e.g., external commands....)
}