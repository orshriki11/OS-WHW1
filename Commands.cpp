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
#include <dirent.h>
#include <fcntl.h>
#include "signals.h"
//#include <sys/types.h>



using namespace std;

const std::string WHITESPACE = " \n\r\t\f\v";

static const array<string, 13> reservedCommands{"chprompt", "showpid", "pwd", "cd", "jobs", "fg", "quit", "kill", "alias", "unalias", "listdir", "getuser", "watch"};


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
    return string(cmd).substr(first + 1, last - first - 1);

}

string _aliasCmd(const char *cmd){
    unsigned last = string(cmd).find_last_of('\'');
    unsigned first = string(cmd).find_first_of('\'');
    return string(cmd).substr(first + 1, last - first- 1);

}

bool aliasCheck(const char *cmd, std::string *aliasCmd){
    SmallShell &smash = SmallShell::getInstance();
    int argsCount;
    char **args = _initArgs(cmd, &argsCount);
    std::string cmdString(cmd);
    std::string tmpCmd;
    //strcpy(args[i], s.c_str());
    for(auto& alias : smash.aliasList) {
        if(string(args[0]).compare(alias.aliasName) == 0) {;
            *aliasCmd = string(cmd);
            aliasCmd->replace(aliasCmd->find(alias.aliasName),alias.aliasName.length(), alias.commandLine);
            return true;
        }
    }
    return false;
}

// TODO: Add your implementation for classes in Commands.h


Command::Command(const char *cmd_line) : cmd_line(cmd_line) {}

BuiltInCommand::BuiltInCommand(const char *cmd_line) : Command(cmd_line) {}

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
    SmallShell &smash = SmallShell::getInstance();
    cout << "smash pid is " << smash.pid << endl;
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
ChangeDirCommand::ChangeDirCommand(const char *cmd_line, char **plastPwd) : BuiltInCommand(cmd_line), plastPwd(plastPwd) {}

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
            if (!(*plastPwd)) {
                std::cerr << "smash error: cd: OLDPWD not set" << std::endl;
                free(buffer);
            } else {
                if (chdir(*plastPwd) == -1) {
                    perror("smash error: chdir failed");
                    free(buffer);
                    freeArgs(args, num_of_args);
                    return;
                } else {
                    if (*plastPwd) {
                        free(*plastPwd);
                    }
                    *plastPwd = buffer;
                }
            }
        } else {
            if (chdir(args[1]) == -1) {
                perror("smash error: chdir failed");
                freeArgs(args, num_of_args);
                return;
            } else {
                if (*plastPwd) {
                    free(*plastPwd);
                }
                *plastPwd = buffer;
            }
        }
    }
    freeArgs(args, num_of_args);
}

JobsList::JobEntry::JobEntry(int jobID, int jobPID, std::string command, bool isStopped) : jobID(jobID), jobPID(jobPID), command(command), isStopped(isStopped) {}

JobsList::JobsList() : jobsList(), maxJobID(0), jobsCount(0) {}

JobsCommand::JobsCommand(const char *cmd_line, JobsList *jobs) : BuiltInCommand(cmd_line), jobs(jobs) {}

void JobsCommand::execute()
{

    SmallShell &smash = SmallShell::getInstance();
    if (!smash.isFork)
        smash.jobsList.removeFinishedJobs();



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
    int lastID = -1;
    for(auto& job : jobsList) {
        if(job.jobID > lastID)
            lastID = job.jobID;
    }
    *lastJobId = lastID;
    return getJobById(lastID);
}

void JobsList::removeJobById(int jobId) {
    for(auto it = jobsList.begin(); it != jobsList.end(); ++it) {
        auto job = *it;
        if(job.jobID == jobId) {
            //auto jobToErase = job;

            jobsList.erase(it);
            jobsCount--;
            break;
        }
    }
    int maxID = 0;

    for(auto& job : jobsList) {
        if(job.jobID > maxID)
            maxID = job.jobID;
    }

    maxJobID = maxID;
}

void JobsList::killAllJobs() {
    for(auto& job : jobsList) {
        cout << job.jobPID << ": " << job.command << endl;
        kill(job.jobPID, SIGKILL);
    }
    jobsCount = 0;
    maxJobID = 0;
}
void JobsList::printJobsList() {
    for(auto& job : jobsList) {
        cout << "[" << job.jobID << "] " << job.command << endl;
    }
}

JobsList::JobEntry *JobsList::getLastStoppedJob(int *jobId) {
    int maxStopped = -1;
    for(auto& job : jobsList) {
        if(job.isStopped)
            maxStopped = job.jobID;
    }
    *jobId = maxStopped;
    return getJobById(maxStopped);
}

void JobsList::removeFinishedJobs() {
    if(jobsList.empty()) {
        maxJobID = 0;
        jobsCount = 0;
        return;
    }
    SmallShell &smash = SmallShell::getInstance();

    for (auto it = jobsList.begin(); it != jobsList.end(); ++it) {
        if(!smash.pipe) {
            auto job = *it;
            int pidStatus;
            int wait = waitpid(job.jobPID, &pidStatus, WNOHANG);
            if(wait == job.jobPID) {
                jobsList.erase(it);
                jobsCount--;
                --it;
            }
            else if(wait < 0) {
                perror("smash error: waitpid failed");
                return;
            }
        }
    }

    int maxID = 0;

    for(auto& job : jobsList) {
        if(job.jobID > maxID)
            maxID = job.jobID;
    }

    maxJobID = maxID;

}
void JobsList::addJob(Command *cmd, pid_t pid, bool isStopped) {
    removeFinishedJobs();
    std::string originalCmd = cmd->originalCmd;

    jobsList.push_back(JobEntry(++maxJobID, pid, originalCmd, isStopped));
    jobsCount++;
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
            if(jobID < 0)
                throw std::exception();
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
        cout << job->command << " " << job->jobPID << endl;
        smash.currentCmd = job->command;
        jobs->removeJobById(jobID);
        smash.fgJobID = jobID;

        if(waitpid(jobPID, &pStatus, WUNTRACED) == -1)
        {
            perror("smash error: waitpid failed");
            freeArgs(args, argsCount);
            smash.currentProcess = -1;
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

}

//Quit Command
QuitCommand::QuitCommand(const char *cmd_line, JobsList *jobs)  : BuiltInCommand(cmd_line) , jobs(jobs) {}

void QuitCommand::execute() {
    int argsCount;
    char **args = _initArgs(this->cmd_line, &argsCount);
    SmallShell &smash = SmallShell::getInstance();

    if(argsCount >= 2 && string(args[1]).compare("kill") == 0) {
        cout << "smash: sending SIGKILL signal to " << jobs->jobsCount << " jobs:" << endl;
        smash.jobsList.killAllJobs();
    }
    freeArgs(args, argsCount);
    exit(0);
    //the command will exit the smash
    //If kill arg is specified then smash should send SIGKILL all of unfinished
    //jobs before exiting
    //If kill was specified then print number of jobs killed, their PIDS and commandlines.
}


KillCommand::KillCommand(const char *cmd_line, JobsList *jobs)  : BuiltInCommand(cmd_line) , jobs(jobs){}

void KillCommand::execute() {
    int argsCount;
    char **args = _initArgs(this->cmd_line, &argsCount);
    //SmallShell &smash = SmallShell::getInstance();

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
        if (JobsList::JobEntry *job = jobs->getJobById(jobID)) {
            int jobPID = job->jobPID;

            cout << "signal number " << sigNum << " was sent to pid " << jobPID << endl;
            if(kill(jobPID, sigNum) == -1) {
                perror("smash error: kill failed");
                freeArgs(args, argsCount);
                return;
            }

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
    string cmdString(_trim(string(cmd_line)));
    char cmdCopy[COMMAND_MAX_LENGTH];
    strcpy(cmdCopy, cmdString.c_str());
    if(_isBackgroundComamnd(cmd_line)){
        _removeBackgroundSign(cmdCopy);
    }

    cmdString = _trim(cmdCopy);
    if(argsCount == 1) {
        for(auto& alias : smash.aliasList) {
            cout << alias.aliasName << "='" << alias.commandLine << "'" << endl;
        }
    }
    if(argsCount >= 2)
    {
        if(!regex_match(cmdString.c_str(),alias_expr))
        {
            cerr << "smash error: alias: invalid alias format" << endl;
            return;
        }
        string aliasName = _aliasName(cmd_line);
        string aliasCmd = _aliasCmd(cmd_line);
        for(auto& alias : smash.aliasList) {
            if (alias.aliasName.compare(aliasName) == 0) {
                cerr << "smash error: alias: " << aliasName << " already exists or is a reserved command" << endl;
                return;
            }
        }
        for(auto& rcmd : reservedCommands) {
            if (rcmd.compare(aliasName) == 0) {
                    cerr << "smash error: alias: " << rcmd << " already exists or is a reserved command" << endl;
                    return;
            }
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
    freeArgs(args, argsCount);
}

unaliasCommand::unaliasCommand(const char *cmd_line)  : BuiltInCommand(cmd_line) {
}

void unaliasCommand::execute() {
    int argsCount;
    char **args = _initArgs(this->cmd_line, &argsCount);
    SmallShell &smash = SmallShell::getInstance();
    if(argsCount >= 2) {
        bool notFound = true;
        for(int i=1; i < argsCount; i++) {
            for (auto it = smash.aliasList.begin(); it != smash.aliasList.end(); it++) {
                if(it->aliasName.compare(string(args[i])) == 0) {
                    auto alias = *it;
                    smash.aliasList.erase(it);
                    //--it;
                    notFound = false;
                    break;
                }
            }
            if(notFound) {
                cerr << "smash error: unalias: " << args[i] << " alias does not exist" << endl;
                freeArgs(args, argsCount);
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
    bool isBg = _isBackgroundComamnd(cmdCopy);
    bool isComplex = false;
    if(string(cmdCopy).find("*") != string::npos || string(cmdCopy).find("?") != string::npos) {
        isComplex = true;
    }
    if(isBg){
        strcpy(cmdCopy, cmdString.c_str());
        _removeBackgroundSign(cmdCopy);
        cmdString = cmdCopy;
    }
    int argsCount;
    char **args = _initArgs(cmdString.c_str(), &argsCount);
    SmallShell &smash = SmallShell::getInstance();
    pid_t pid = fork();

    if (pid < 0)
    {
        perror("smash error: fork failed");
        freeArgs(args, argsCount);
        return;
    } else if (pid == 0) {
        smash.isFork = true;
        smash.currentProcess = -1;
        if (setpgrp() == -1) {
            perror("smash error: setpgrp failed");
            exit(0);
        }
        if(isComplex) {
            if (execlp("/bin/bash", "/bin/bash", "-c", cmdCopy, nullptr) == -1) {
                perror("smash error: execlp failed");
                exit(0);
            }
        } else {
            if(execvp(args[0], args) == -1) {
                perror("smash error: execvp failed");
                exit(0);

            }

        }
        freeArgs(args, argsCount);
        delete this;
        exit(0);
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

//////////////////////////////-------------Special-Commands-------------//////////////////////////////

RedirectionCommand::RedirectionCommand(const char *cmd_line) : Command(cmd_line) {}

void RedirectionCommand::execute()
{
    int file_descriptor;
    string modified_command;
    int redirection_position, length;

    // Check for background execution character '&' and handle it
    string cmd_str(cmd_line);
    size_t bg_pos = cmd_str.find("&");
    if (bg_pos != string::npos) {
        modified_command = cmd_str.substr(0, bg_pos);
    } else {
        modified_command = cmd_str;
    }

    // Determine the type of redirection and set the respective position and length
    size_t append_pos = modified_command.find(">>");
    size_t overwrite_pos = modified_command.find(">");
    if (append_pos != string::npos) {
        this->redirect_type = ">>";
        redirection_position = append_pos;
        length = 2;
    } else {
        this->redirect_type = ">";
        redirection_position = overwrite_pos;
        length = 1;
    }

    // Extract command and file name
    this->command = modified_command.substr(0, redirection_position);
    this->file_name = _trim(modified_command.substr(redirection_position + length));

    SmallShell &shell_instance = SmallShell::getInstance();
    pid_t child_pid = fork();
    if (child_pid < 0) {
        perror("smash error: fork failed");
        return;
    }

    // In child process
    if (child_pid == 0) {
        shell_instance.isFork = true;
        shell_instance.currentProcess = -1;

        if (setpgrp() == -1) {
            perror("smash error: setpgrp failed");
            exit(0);
        }

        if (close(1) == -1) {
            perror("smash error: close failed");
            exit(0);
        }

        // Open file for redirection
        if (this->redirect_type == ">") {
            file_descriptor = open(this->file_name.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0666);
        } else {
            file_descriptor = open(this->file_name.c_str(), O_WRONLY | O_APPEND | O_CREAT, 0666);
        }

        if (file_descriptor == -1) {
            perror("smash error: open failed");
            exit(0);
        }

        shell_instance.fd = file_descriptor;
        shell_instance.executeCommand(command.c_str());

        if (close(file_descriptor) == -1) {
            perror("smash error: close failed");
            exit(0);
        }

        exit(0);
    }

    // In parent process
    int wait_status;
    if (waitpid(child_pid, &wait_status, WUNTRACED) == -1) {
        perror("smash error: waitpid failed");
        return;
    }
}

PipeCommand::PipeCommand(const char *cmd_line) : Command(cmd_line) {}

void PipeCommand::execute()
{
    string primaryCmd;
    string secondaryCmd;
    string pipeSymbol;
    string commandStr = string(cmd_line);
    int splitPoint;
    int pipeEnds[2];
    pipe(pipeEnds);
    SmallShell &smashInstance = SmallShell::getInstance();

    // Identify and split based on the pipe type
    size_t pipeErrPos = commandStr.find("|&");
    size_t pipeOutPos = commandStr.find("|");
    if (pipeErrPos != string::npos) {
        splitPoint = pipeErrPos;
        pipeSymbol = "|&";
        primaryCmd = commandStr.substr(0, splitPoint);
        secondaryCmd = commandStr.substr(splitPoint + 2);
    } else {
        pipeSymbol = "|";
        splitPoint = pipeOutPos;
        primaryCmd = commandStr.substr(0, splitPoint);
        secondaryCmd = commandStr.substr(splitPoint + 1);
    }

    // First child process creation
    pid_t firstChildPID = fork();
    if (firstChildPID < 0) {
        perror("smash error: fork failed");
        close(pipeEnds[0]);
        close(pipeEnds[1]);
        return;
    }

    if (firstChildPID == 0) {
        if (setpgrp() == -1) {
            perror("smash error: setpgrp failed");
            close(pipeEnds[0]);
            close(pipeEnds[1]);
            exit(1);
        }

        if (pipeSymbol == "|") {
            if (dup2(pipeEnds[1], STDOUT_FILENO) == -1) {
                perror("smash error: dup2 failed");
                close(pipeEnds[0]);
                close(pipeEnds[1]);
                exit(1);
            }
        } else {
            if (dup2(pipeEnds[1], STDERR_FILENO) == -1) {
                perror("smash error: dup2 failed");
                close(pipeEnds[0]);
                close(pipeEnds[1]);
                exit(1);
            }
        }

        close(pipeEnds[0]);
        close(pipeEnds[1]);
        smashInstance.pipe = true;
        smashInstance.isFork = true;
        smashInstance.executeCommand(primaryCmd.c_str());
        exit(0);
    }

    // Second child process creation
    pid_t secondChildPID = fork();
    if (secondChildPID < 0) {
        perror("smash error: fork failed");
        close(pipeEnds[0]);
        close(pipeEnds[1]);
        return;
    }

    if (secondChildPID == 0) {
        if (setpgrp() == -1) {
            perror("smash error: setpgrp failed");
            close(pipeEnds[0]);
            close(pipeEnds[1]);
            exit(1);
        }

        if (dup2(pipeEnds[0], STDIN_FILENO) == -1) {
            perror("smash error: dup2 failed");
            close(pipeEnds[0]);
            close(pipeEnds[1]);
            exit(1);
        }

        close(pipeEnds[0]);
        close(pipeEnds[1]);
        smashInstance.pipe = true;
        smashInstance.isFork = true;
        smashInstance.executeCommand(secondaryCmd.c_str());
        exit(0);
    }

    close(pipeEnds[0]);
    close(pipeEnds[1]);

    if (waitpid(firstChildPID, nullptr, WUNTRACED) == -1) {
        perror("smash error: waitpid failed");
        return;
    }

    if (waitpid(secondChildPID, nullptr, WUNTRACED) == -1) {
        perror("smash error: waitpid failed");
        return;
    }
}

WatchCommand::WatchCommand(const char *cmd_line) : Command(cmd_line) {}

void WatchCommand::execute() {
    int interval = 2;
    //bool intervalProvided = false;
    int argsCount;
    bool isBg = _isBackgroundComamnd(cmd_line);
    //bool intervalProvided = false;
    string cmdString(cmd_line);
    char **args = _initArgs(this->cmd_line, &argsCount);
    char cmdCopy[COMMAND_MAX_LENGTH];
    SmallShell &smash = SmallShell::getInstance();
    if(isBg)
    {

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
        //intervalProvided = true;
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
        unsigned start = tmpCmd.find_first_of(WHITESPACE);
        tmpCmd = _trim(tmpCmd.substr(start + string(args[1]).length()- 1));
        //intervalProvided = true;
    }

    pid_t pid = fork();
    if(pid < 0){
        perror("smash error: fork failed");
        freeArgs(args, argsCount);
        return;
    } else if(pid == 0){
        smash.isFork = true;
        if(setpgrp() == -1){
            perror("smash error: setpgrp failed");
            exit(0);
        }
        if(isBg){
            close(1);
        }
        int i = 0;
        smash.watchProcess = pid;
        while(!smash.sigStop) {
            system("clear");
            smash.executeCommand(tmpCmd.c_str());
            i++;
            //signal(SIGINT, ctrlCHandler);
            sleep(interval);
        }
        smash.sigStop = false;
        //smash.watchProcess = -1;
        freeArgs(args, argsCount);
        delete this;
        exit(0);
        //return;
    } else if (!isBg){
        smash.watchProcess = pid;
        int pstatus;
        if (waitpid(pid, &pstatus, WUNTRACED) == -1)
        {
            perror("smash error: waitpid failed");
            smash.watchProcess = -1;
            return;
        }
        smash.watchProcess = -1;
        smash.sigStop = false;
    } else {
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
    char cmdCopy[COMMAND_MAX_LENGTH];
    if(_isBackgroundComamnd(cmd_line))
    {

        strcpy(cmdCopy, cmdString.c_str());
        _removeBackgroundSign(cmdCopy);
        cmdString = cmdCopy;
    }
    char **args = _initArgs(this->cmd_line, &argsCount);

    if(argsCount != 2){
        std::cerr << "smash error: getuser: too many arguments" << std::endl;
        freeArgs(args, argsCount);
        return;
    }
    //SmallShell &smash = SmallShell::getInstance();

    //pid_t pid = ;
    if (access(("/proc/" + string(args[1])).c_str(), R_OK) == -1)
    {
        cerr << "smash error: getuser: process " << args[1] << " does not exist" << endl;
        return;
    }
    std::string filepath = "/proc/" + string(args[1]) + "/status";
    int status = open(filepath.c_str(), O_RDONLY);
    if(status == -1) {
        std::cerr << "smash error: getuser: process " << args[1] <<" does not exist" << std::endl;
        return;
    }
    //std::string line;
    char line[COMMAND_MAX_LENGTH];
    ssize_t bytesRead;
    uid_t uid = 0;
    gid_t gid = 0;
    while ((bytesRead = read(status, line, sizeof(line) - 1)) > 0) {
        //std::string stringLine(line);
//        if (stringLine.find("Uid:") != string::npos) {
//            //line = line.substr(line.find_first_of(":") + 1);
//            uid = stoi(stringLine.c_str() + 5);
//        }
//        if (stringLine.find("Gid:") != string::npos) {
//            //line = line.substr(line.find_first_of(":") + 1);
//            gid = stoi(stringLine.c_str() + 5);
//        }
        line[bytesRead] = '\0';

        if (strncmp(line, "Uid:", 4) == 0)
        {
            uid = stoi(line + 5);
        }
        else if (strncmp(line, "Gid:", 4) == 0)
        {
            gid = stoi(line + 5);
        }
        if (uid != 0 && gid != 0){
            break;
        }
    }
    close(status);

    string username = getUsernameFromUID(uid);
    string groupname = getGroupNameFromGID(gid);
    cout << "User: " << username << endl;
    cout << "Group: " << groupname << endl;
    freeArgs(args, argsCount);
    return;
}

ListDirCommand::ListDirCommand(const char *cmd_line) : BuiltInCommand(cmd_line) {}

void ListDirCommand::execute() {
    int argsCount;
    string cmdString(cmd_line);
    char *path;
    if(_isBackgroundComamnd(cmdString.c_str()))
    {
        char cmdCopy[COMMAND_MAX_LENGTH];
        strcpy(cmdCopy, cmdString.c_str());
        _removeBackgroundSign(cmdCopy);
        cmdString = cmdCopy;
    }
    char **args = _initArgs(cmdString.c_str(), &argsCount);
    //bool currDir = false;
    char *buffer = (char *)malloc(COMMAND_MAX_LENGTH);
    if(argsCount > 2){
        std::cerr << "smash error: listdir: too many arguments" << std::endl;
        freeArgs(args, argsCount);
        return;
    } else if(argsCount == 1){
        //currDir = true;
        if(getcwd(buffer, COMMAND_MAX_LENGTH) == NULL){
            perror("smash error: getcwd failed");
            freeArgs(args, argsCount);
            return;
        }
        path = buffer;
        //free(buffer);
        //path = get_current_dir_name();
    } else{
        path = args[1];
    }
    int fd = open(path, O_RDONLY | O_DIRECTORY);
    if (fd == -1)
    {
        perror("smash error: open failed");
        freeArgs(args, argsCount);
        free(buffer);
        return;
    }
    char buf[8192];
    vector<string> printingOrder = {"file", "directory", "link"};
    map<char, string> typeName = {{DT_REG, printingOrder[0]}, {DT_DIR, printingOrder[1]}, {DT_LNK, printingOrder[2]}};
    map<string, vector<string>> filesMap;
    while(true) {
        int read = syscall(SYS_getdents, fd, buf, 8192);
        if (read == -1) {
            perror("smash error: Failed to read directory");
            close(fd);
            freeArgs(args, argsCount);
            free(buffer);
            return;
        } else if (read == 0) {
            break;
        }
        for (int bpos = 0; bpos < read;) {
            struct linux_dirent *d = (struct linux_dirent *)(buf + bpos);
            char d_type = *(buf + bpos + d->d_reclen - 1);
            string name = _trim(string(d->d_name));
            if (name.find(".") != 0) {
                string type;
                if (typeName.find(d_type) != typeName.end()) {
                    type = typeName[d_type];
                    if (filesMap.find(type) != filesMap.end()) {
                        filesMap[type].push_back(name);
                    } else {
                        filesMap[type] = {name};
                    }
                }
            }
            bpos += d->d_reclen;
        }
    }

    for(string &type : printingOrder){
        if(filesMap.find(type) != filesMap.end()){
            auto namesArr = filesMap[type];
            sort(namesArr.begin(), namesArr.end());
            for(string &name : namesArr){
                cout << type << ": " << name << endl;
            }
        }
    }

    freeArgs(args, argsCount);
    free(buffer);
    close(fd);
    return;

}

SmallShell::SmallShell() : pid(getpid()), currentProcess(-1),watchProcess(-1), currentCmd(""), fgJobID(-1),isFork(false), pipe(false),prev_directory(nullptr),smash_prompt("smash"),sigStop(false) {
    //pid = -1;
}

SmallShell::~SmallShell() {
    if(pid != -1 && !isFork){
        if(kill(pid, SIGKILL) == -1){
            perror("smash error: kill failed");
        }
    }
// TODO: add your implementation
}

JobsList::~JobsList()
{
}

Command::~Command()
{
}

/**
* Creates and returns a pointer to Command class which matches the given command line (cmd_line)
*/
Command *SmallShell::CreateCommand(const char *cmd_line) {

  string cmd_s = _trim(string(cmd_line));
  string firstArg = cmd_s.substr(0, cmd_s.find_first_of(" \n"));

  string aliasCmd;

  if (firstArg.compare("alias") == 0)
  {
      aliasCmd = cmd_s.substr(cmd_s.find_first_of(" \n") + 1);
  }
  if (firstArg.compare("alias") == 0 || firstArg.compare("alias&") == 0) {
      return new aliasCommand(cmd_line);
  }
  else if (string(cmd_line).find("|") != string::npos || string(cmd_line).find("|&") != string::npos)
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
      return new ChangeDirCommand(cmd_line, &prev_directory);
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
  }else if (firstArg.compare("watch") == 0 || firstArg.compare("watch&") == 0)
  {
      return new WatchCommand(cmd_line);
  } else if (firstArg.compare("getuser") == 0 || firstArg.compare("getuser&") == 0){
      return new GetUserCommand(cmd_line);
  } else if(firstArg.compare("listdir") == 0 || firstArg.compare("listdir&") == 0 ){
      return new ListDirCommand(cmd_line);
  } else if (firstArg.compare("unalias") == 0 || firstArg.compare("unalias&") == 0) {
      return new unaliasCommand(cmd_line);
  }
  else if (string(cmd_line).find(">") != string::npos || string(cmd_line).find(">>") != string::npos) {
      return new RedirectionCommand(cmd_line);
  }
  else {

    return new ExternalCommand(cmd_line);
  }
    return nullptr;
}

void SmallShell::executeCommand(const char *cmd_line) {
    if (string(cmd_line).find_first_not_of(WHITESPACE) == string::npos) {
        return;
    }
    if(!isFork)
        this->jobsList.removeFinishedJobs();
    string aliasCmd;
    string cmd_original(cmd_line);
    if(aliasCheck(cmd_line, &aliasCmd))
    {
        cmd_line = aliasCmd.c_str();
    }
    Command* cmd = CreateCommand(cmd_line);
    cmd->originalCmd = cmd_original;
    cmd->execute();


}