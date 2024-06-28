#include <unistd.h>
#include <string.h>
#include <iostream>
#include <vector>
#include <sstream>
#include <sys/wait.h>
#include <iomanip>
#include "Commands.h"

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


// TODO: Add your implementation for classes in Commands.h

JobsList SmallShell::jobsList;
pid_t SmallShell::pid = getppid();

JobsList::JobEntry *JobsList::getJobById(int jobId) {
    for(auto& job : jobsList) {
        if(job.jobID = jobId)
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
        if(job.jobID = jobId) {
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

}

void ForegroundCommand::execute() {
    char **args = (char **) malloc(COMMAND_MAX_ARGS * sizeof(char **));
    int argsCount = _parseCommandLine(this->cmd_line, args);
    int jobID;
    JobsList::JobEntry *job;
    SmallShell &smash = SmallShell::getInstance();
    if(argsCount == 2)
    {
        try
        {
            jobID = stoi(args[1]);
        }
        catch(const std::exception& e)
        {
            std::cerr << e.what() << '\n';
        }
        job = jobs->getJobById(jobID);
        

    }
    else if(argsCount == 1)
    {
        job = jobs->getLastJob(&jobID);
    }
    else
    {
        cerr << "smash error: fg: invalid arguments" << endl;
        return;
    }

    int jobPID = job->jobPID;
    int pStatus;
    smash.currentProcess = jobPID;
    smash.currentCmd = job->command;
    smash.fgJobID = jobID;
    jobs->removeJobById(jobID);
    if(waitpid(jobPID, &pStatus, WUNTRACED) == SYS_FAIL)
    {

        // TODO: output and etc
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
    char **args = (char **) malloc(COMMAND_MAX_ARGS * sizeof(char **));
    int argsCount = _parseCommandLine(this->cmd_line, args);
    //the command will exit the smash
    //If kill arg is specified then smash should send SIGKILL all of unfinished
    //jobs before exiting
    //If kill was specified then print number of jobs killed, their PIDS and commandlines.
}


KillCommand::KillCommand(const char *cmd_line, JobsList *jobs)  : BuiltInCommand(cmd_line) {}

void KillCommand::execute() {
    //
}


aliasCommand::aliasCommand(const char *cmd_line)  : BuiltInCommand(cmd_line) {
}

void aliasCommand::execute() {
    char **args = (char **) malloc(COMMAND_MAX_ARGS * sizeof(char **));
    int argsCount = _parseCommandLine(this->cmd_line, args);
    SmallShell &smash = SmallShell::getInstance();
    if(argsCount == 1) {
        for(auto& alias : smash.aliasList) {
            cout << alias.alias << "='" << alias.commandLine << "'" << endl;
        }
    }
    if(argsCount == 2)
    {

    }
}

unaliasCommand::unaliasCommand(const char *cmd_line)  : BuiltInCommand(cmd_line) {
}

void unaliasCommand::execute() {
    char **args = (char **) malloc(COMMAND_MAX_ARGS * sizeof(char **));
    int argsCount = _parseCommandLine(this->cmd_line, args);
    SmallShell &smash = SmallShell::getInstance();



    if(argsCount == 2) {
        for (auto it = smash.aliasList.begin(); it != smash.aliasList.end(); ++it) {
            if(std::equal(it->alias, args[1])) {
                auto job = *it;{
                    smash.aliasList.erase(it);
                    return;
                }
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