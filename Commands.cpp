#include <unistd.h>
#include <string.h>
#include <iostream>
#include <vector>
#include <list>
#include <sstream>
#include <sys/wait.h>
#include <iomanip>
#include "Commands.h"
#include <regex>
#include <sys/syscall.h>
#include <pwd.h>
#include <grp.h>
#include <limits.h>
#include <fstream>

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

void freeArgs(char **args, int arg_num)
{
    if (!args)
        return;

    for (int i = 0; i < arg_num; i++) {
        if (args[i])
            free(args[i]);
    }
    free(args);
}

std::string getCurrentPath()
{
    char temp[PATH_MAX];
    if (getcwd(temp, sizeof(temp)) == nullptr) {
        perror("getcwd");
        return "";
    }
    return std::string(temp);
}

bool is_number(const std::string& s)
{
    std::string::const_iterator it = s.begin();
    while (it != s.end() && std::isdigit(*it)) ++it;
    return !s.empty() && it == s.end();
}

string _aliasName(const char *cmd){
    unsigned first = string(cmd).find(' ');
    unsigned last = string(cmd).find('=');
    return string(cmd).substr(first, last);

}

string _aliasCmd(const char *cmd){
    unsigned last = string(cmd).find_last_of('\'');
    unsigned first = string(cmd).find_first_of('\'');
    return string(cmd).substr(first, last);

}

bool aliasCheck(const char *cmd, std::string *aliasCmd){
    SmallShell &smash = SmallShell::getInstance();
    bool isAlias = false;
    std::string cmdString(cmd);
    //string potentialAlias = cmdString.substr(0, );
    std::string tmpCmd;
    //strcpy(args[i], s.c_str());
    for(auto& alias : smash.aliasList) {
        if(cmdString.find_first_of(alias.aliasName) != std::string::npos) {;
            isAlias = true;
            *aliasCmd = string(cmd);
            aliasCmd = &aliasCmd->replace(aliasCmd->find(cmdString),alias.aliasName.length(), alias.commandLine);
            return true;
        }
    }
    return false;
}

// TODO: Add your implementation for classes in Commands.h

//////////////////////////////-------------Built-in Commands-------------//////////////////////////////

/* Chprompt Command - chprompt command will allow the user to change the prompt displayed by
                      the smash while waiting for the next command.*/

ChpromptCommand::ChpromptCommand(const char *cmd_line) : BuiltInCommand(cmd_line) {}

void ChpromptCommand::execute()
{
    int num_of_args;
    char** args = _initArgs(cmd_line, &num_of_args);
    if (args == nullptr) {
        perror("smash error : malloc failed");
        return;
    }
    SmallShell &shell = SmallShell::getInstance();
    if (num_of_args == 1) {
        shell.smash_prompt = "smash";
    } else {
        shell.smash_prompt = args[1];
    }
    freeArgs(args, num_of_args);
}

/* ShowPidCommand - showpid command prints the smash PID.*/

ShowPidCommand::ShowPidCommand(const char *cmd_line) : BuiltInCommand(cmd_line) {}

void ShowPidCommand::execute()
{
    cout << "smash pid is " << SmallShell::getInstance().pid << endl;
}

GetCurrDirCommand::GetCurrDirCommand(const char *cmd_line) : BuiltInCommand(cmd_line) {}

void GetCurrDirCommand::execute()
{
    string path = getCurrentPath();
    if(!path.empty())
        cout << path << endl;
}

/* CD Command - The cd (Change Directory) command takes a single argument <path> that specifies either a
                relative or full path to change the current working directory to.
                There's a special argument, `-`, that cd can accept. When `-` is the only argument provided
                to the cd command, it instructs the shell to change the current working directory to the
                last working directory.*/
ChangeDirCommand::ChangeDirCommand(const char *cmd_line, char **Penultimate) : BuiltInCommand(cmd_line), Penultimate(Penultimate) {}

void ChangeDirCommand::execute()
{

    int num_of_args;
    char **args = _initArgs(this->cmd_line, &num_of_args);
    if (!args) {
        perror("smash error: malloc failed");
        return;
    }

    if (num_of_args > 2) {
        std::cerr << "smash error: cd: too many arguments" << std::endl;
    }
    else if (num_of_args == 1) {
        std::cerr << "smash error: cd: no arguments" << std::endl;
    }
    else {
        long max_size = pathconf(".", _PC_PATH_MAX);
        char *buffer = (char *) malloc((size_t) max_size);
        if (!buffer) {
            perror("smash error: malloc failed");
            freeArgs(args, num_of_args);
            return;
        }

        if (getcwd(buffer, (size_t) max_size) == nullptr) {
            perror("smash error: getcwd failed");
            free(buffer);
            freeArgs(args, num_of_args);
            return;
        }

        std::string next = args[1];

        if (next == "-") {
            if (!(*Penultimate)) {
                std::cerr << "smash error: cd: OLDPWD not set" << std::endl;
                free(buffer);
            } else {
                if (chdir(*Penultimate) == -1) {
                    perror("smash error: chdir failed");
                    free(buffer);
                    freeArgs(args, num_of_args);
                    return;
                } else {
                    if (*Penultimate) {
                        free(*Penultimate);
                    }
                    *Penultimate = buffer;
                }
            }
        } else {
            if (chdir(args[1]) == -1) {
                perror("smash error: chdir failed");
                freeArgs(args, num_of_args);
                return;
            } else {
                if (*Penultimate) {
                    free(*Penultimate);
                }
                *Penultimate = buffer;
            }
        }
    }
    freeArgs(args, num_of_args);
}

JobsCommand::JobsCommand(const char *cmd_line, JobsList *jobs) : BuiltInCommand(cmd_line), jobs(jobs) {}

void JobsCommand::execute()
{

    //SmallShell &smash = SmallShell::getInstance();

    jobs->removeFinishedJobs();

    jobs->printJobsList();
}

//JobsList SmallShell::jobsList;
//pid_t SmallShell::pid = getppid();

JobsList::JobEntry *JobsList::getJobById(int jobId) {
    for(auto& job : jobsList) {
        if (job.jobID == jobId) {
            return &job;
        }
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
            //auto jobToErase = job;
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
        cout << "[" << job.jobID << "] " << job.command << endl;
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
void JobsList::addJob(Command *cmd, pid_t pid, bool isStopped) {
    removeFinishedJobs();
    std::string cmd_line(cmd->getCmdLine());
    //SmallShell &smash = SmallShell::getInstance();
//    if (!_isBackgroundComamnd(cmd->getCmdLine().c_str())) {
//        for(auto it = jobsList.begin(); it != jobsList.end(); ++it){
//            auto job = *it;
//            if(job.jobID > smash.fgJobID) {
//                jobsList.insert(job, JobEntry(smash.fgJobID, job.jobPID, cmd_line, isStopped));
//                break;
//            }
//        }
//
//        return;
//    }
    jobsList.push_back(JobEntry(maxJobID++, pid, cmd_line, isStopped));
    //maxJobID++;
}





ForegroundCommand::ForegroundCommand(const char *cmd_line, JobsList *jobs) : BuiltInCommand(cmd_line),
jobs(jobs){}


void ForegroundCommand::execute() {
    int argsCount;
    char **args = _initArgs(cmd_line, &argsCount);
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
        if(job->isStopped && kill(jobPID, SIGCONT) == -1)
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
        if(waitpid(jobPID, &pStatus, WUNTRACED) == -1)
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
    freeArgs(args, argsCount);
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

            if(kill(jobPID, sigNum) == -1) {
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
        string aliasName = _aliasName(cmd_line);
        string aliasCmd = _aliasCmd(cmd_line);
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
    //wowss
    freeArgs(args, argsCount);
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
                if(it->aliasName.compare(string(args[i])) == 0) {
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
    freeArgs(args, argsCount);

}

ExternalCommand::ExternalCommand(const char *cmd_line) : Command(cmd_line) {}

void ExternalCommand::execute() {
    string trimCmd = _trim(string(cmd_line));
    char cmdCopy[COMMAND_MAX_LENGTH];
    strcpy(cmdCopy, trimCmd.c_str());
    string cmdString(cmdCopy);
    int argsCount;
    char **args = _initArgs(this->cmd_line, &argsCount);


    bool isBg = _isBackgroundComamnd(cmd_line);
    bool isComplex = false;
    if(string(cmdCopy).find("*") != string::npos || string(cmdCopy).find("?") != string::npos) {
        isComplex = true;
    }
    if(isBg){
        char cmdCopy[COMMAND_MAX_LENGTH];
        strcpy(cmdCopy, cmdString.c_str());
        _removeBackgroundSign(cmdCopy);
        cmdString = cmdCopy;
    }
    SmallShell &smash = SmallShell::getInstance();
    //char executable[] = "/bin/bash";
    //char executable_name[] = "/bin/bash";
    //char flag[] = "-c";
    //char *fork_args[] = {"/bin/bash", flag, cmdCopy, NULL};
    pid_t pid = fork();

    if (pid < 0)
    {
        perror("smash error: fork failed");
        freeArgs(args, argsCount);
        return;
    } else if (pid == 0) {
        if (setpgrp() == -1) {
            perror("smash error: setpgrp failed");
            return;
        }
        if(isComplex) {
            if (execlp("/bin/bash", "/bin/bash", "-c", cmdCopy, NULL) == -1) {
                perror("smash error: execlp failed");
                return;
            }
        } else {
            if(execve(args[0], args, NULL) == -1) {
                perror("smash error: execve failed");
                return;
            }

        }
        freeArgs(args, argsCount);
        return;
    } else {

        if (isBg) {
            smash.jobsList.addJob(this, pid);
        } else {
            smash.currentProcess = pid;
            smash.currentCmd = cmd_line;
            int pstatus;
            if (waitpid(pid, &pstatus, WUNTRACED) == -1) {
                perror("smash error: waitpid failed");
                freeArgs(args, argsCount);
                smash.currentProcess = -1;
                return;
            }
            smash.currentProcess = -1;
        }
    }
    freeArgs(args, argsCount);
}


WatchCommand::WatchCommand(const char *cmd_line) : Command(cmd_line) {}

void WatchCommand::execute() {
    int interval = 2;
    int argsCount;
    bool isBg = _isBackgroundComamnd(cmd_line);
    bool intervalProvided = false;
    string cmdString(cmd_line);
    char **args = _initArgs(this->cmd_line, &argsCount);
    SmallShell &smash = SmallShell::getInstance();
    if(isBg)
    {
        char cmdCopy[COMMAND_MAX_LENGTH];
        strcpy(cmdCopy, cmdString.c_str());
        _removeBackgroundSign(cmdCopy);
        cmdString = cmdCopy;
    }

    string tmpCmd = _trim(_trim(string(cmd_line)).substr(5));

    if(argsCount < 2){
        std::cerr << "smash error: watch: command not specified" << std::endl;
        return;
    }
    if(is_number(args[1])){
        interval = stoi(args[1]);
        if(interval <= 0)
        {
            std::cerr << "smash error: watch: invalid interval" << std::endl;
            return;
        }
        if(argsCount <= 2)
        {
            std::cerr << "smash error: watch: command not specified" << std::endl;
            return;
        }
        size_t start = tmpCmd.find_first_of(WHITESPACE);
        tmpCmd = _trim(tmpCmd.substr(start + sizeof(args[1]) - 1));
        intervalProvided = true;
    }

    pid_t pid = fork();
    if(pid == -1){
        perror("smash error: fork failed");
        return;
    }
    if(pid == 0){
        smash.isFork = true;
        if(setpgrp() == -1){
            perror("smash error: setpgrp failed");
            return;
        }
        if(isBg){
            close(1);
        }
        else
        {
            system("clear");
        }
        while(true) {
            smash.executeCommand(tmpCmd.c_str());
            sleep(interval);
        }
        //return;
    } else if (!isBg){
        smash.pid = pid;
        int pstatus;
        if (waitpid(pid, &pstatus, WUNTRACED) == -1)
        {
            perror("smash error: waitpid failed");
            smash.pid = -1;
            return;
        }
        smash.pid = -1;
    }
    else {
        smash.jobsList.addJob(this, pid);
    }
}



string getUsernameFromUID(uid_t uid) {
    struct passwd *pw = getpwuid(uid);
    if (pw == NULL) {
        return "Unknown";
    }
    return pw->pw_name;
}

string getGroupNameFromGID(gid_t gid) {
    struct group *grp = getgrgid(gid);
    if (grp == NULL) {
        return "Unknown";
    }

    return grp->gr_name;
}

GetUserCommand::GetUserCommand(const char *cmd_line) : BuiltInCommand(cmd_line) {}

void GetUserCommand::execute() {
    int argsCount;
    string cmdString(cmd_line);
    if(_isBackgroundComamnd(cmd_line))
    {
        char cmdCopy[COMMAND_MAX_LENGTH];
        strcpy(cmdCopy, cmdString.c_str());
        _removeBackgroundSign(cmdCopy);
        cmdString = cmdCopy;
    }
    char **args = _initArgs(this->cmd_line, &argsCount);

    if(argsCount > 2){
        std::cerr << "smash error: getuser: too many arguments" << std::endl;
        return;
    }
    SmallShell &smash = SmallShell::getInstance();

    //pid_t pid = ;
    std::string statusFile = "/proc/" + string(args[1]) + "/status";
    ifstream status(statusFile);
    if(status.fail()) {
        std::cerr << "smash error: getuser: process " << args[1] <<" does not exist" << std::endl;
        return;
    }
    std::string line;
    uid_t uid;
    gid_t gid;
    while (getline(status, line)) {
        if (line.find("Uid:") != string::npos) {
            //line = line.substr(line.find_first_of(":") + 1);
            uid = stoi(line.c_str()  + 5);
        }
        if (line.find("Gid:") != string::npos) {
            //line = line.substr(line.find_first_of(":") + 1);
            gid = stoi(line.c_str() + 5);
        }
        if (uid != 0 && gid != 0){
            break;
        }
    }
    //close(status);

    string username = getUsernameFromUID(uid);
    string groupname = getGroupNameFromGID(gid);
    cout << "User: " << username << endl;
    cout << "Group: " << groupname << endl;
    freeArgs(args, argsCount);
    return;
}

SmallShell::SmallShell() : currentProcess(-1), currentCmd(""), fgJobID(-1), isFork(false), isPipe(false) {
// TODO: add your implementation
    pid = -1;
}

SmallShell::~SmallShell() {
    if(pid != -1 && !isFork){
        if(kill(pid, SIGKILL) == -1){
            perror("smash error: kill failed");
        }
    }
// TODO: add your implementation
}

/**
* Creates and returns a pointer to Command class which matches the given command line (cmd_line)
*/
Command *SmallShell::CreateCommand(const char *cmd_line) {

  string cmd_s = _trim(string(cmd_line));
  string firstArg = cmd_s.substr(0, cmd_s.find_first_of(" \n"));

  //bool isAlias = aliasCheck(firstArg);

  //if (firstArg.compare("alias") == 0 || firstWord.compare("alias&") == 0)
  //{
  //    return new aliasCommand(cmd_line);
  //}

  if (string(cmd_line).find("|") != string::npos || string(cmd_line).find("|&") != string::npos)
  {
      return new PipeCommand(cmd_line);
  }

  if (string(cmd_line).find(">") != string::npos || string(cmd_line).find(">>") != string::npos)
  {
      return new RedirectionCommand(cmd_line);
  }

  if(firstArg.compare("chprompt") == 0 || firstArg.compare("chprompt&") == 0) {
      return new ChpromptCommand(cmd_line);
  }
  else if (firstArg.compare("showpid") == 0 || firstArg.compare("showpid&") == 0) {
      return new ShowPidCommand(cmd_line);
  }
  else if (firstArg.compare("pwd") == 0 || firstArg.compare("pwd&") == 0) {
    return new GetCurrDirCommand(cmd_line);
  }
  //Need to add a proper last directory ref.
  else if (firstArg.compare("cd") == 0 || firstArg.compare("cd&") == 0) {
      return new ChangeDirCommand(cmd_line, &currentDir);
  }
      //Need to add a proper last directory ref.
  else if (firstArg.compare("jobs") == 0 || firstArg.compare("jobs&") == 0) {
      return new JobsCommand(cmd_line, &jobsList);
  }
  else if (firstArg.compare("fg") == 0 || firstArg.compare("fg&") == 0) {
      return new ForegroundCommand(cmd_line, &jobsList);
  }
  else if (firstArg.compare("quit") == 0 || firstArg.compare("quit&") == 0) {
      return new QuitCommand(cmd_line, &jobsList);
  }
  else if (firstArg.compare("kill") == 0 || firstArg.compare("kill&") == 0) {
      return new KillCommand(cmd_line, &jobsList);
  }
  else if (firstArg.compare("alias") == 0 || firstArg.compare("alias&") == 0) {
      return new aliasCommand(cmd_line);
  }
  else if (firstArg.compare("unalias") == 0 || firstArg.compare("unalias&") == 0) {
      return new unaliasCommand(cmd_line);
  }
  else {
      //!!Need to check if command is alias and if so then combine args into it.
      string aliasCmd;
      if(aliasCheck(cmd_line, &aliasCmd)){
          //char* cmd = aliasCmd[0];
          return CreateCommand(aliasCmd.c_str());
      }

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