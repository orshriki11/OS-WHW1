#ifndef SMASH_COMMAND_H_
#define SMASH_COMMAND_H_

#include <vector>
#include <cstring>
#include <string.h>
#include <list>

#define COMMAND_MAX_LENGTH (200)
#define COMMAND_MAX_ARGS (20)


struct linux_dirent {
    unsigned long  d_ino;
    off_t          d_off;
    unsigned short d_reclen;
    char           d_name[];
};

class Command {
protected:
    const char* cmd_line;
public:
    Command(const char *cmd_line);

    virtual ~Command();

    std::string originalCmd;
    virtual void execute() = 0;
    //virtual void prepare();
    //virtual void cleanup();
    std::string getCmdLine() { return std::string(cmd_line); }
};

class BuiltInCommand : public Command {
public:
    BuiltInCommand(const char *cmd_line);

    virtual ~BuiltInCommand() {}
};

class ExternalCommand : public Command {
public:
    ExternalCommand(const char *cmd_line);

    virtual ~ExternalCommand() {}

    void execute() override;
};

//class PipeCommand : public Command {
//
//public:
//    PipeCommand(const char *cmd_line);
//
//    virtual ~PipeCommand() {}
//
//    void execute() override;
//};

class WatchCommand : public Command {

public:
    WatchCommand(const char *cmd_line);

    virtual ~WatchCommand() {}

    void execute() override;
};

class ChangeDirCommand : public BuiltInCommand {
public:
    char **plastPwd;

    ChangeDirCommand(const char *cmd_line, char **plastPwd);

    virtual ~ChangeDirCommand() {}

    void execute() override;
};

class GetCurrDirCommand : public BuiltInCommand {
public:
    GetCurrDirCommand(const char *cmd_line);

    virtual ~GetCurrDirCommand() {}

    void execute() override;
};

class ShowPidCommand : public BuiltInCommand {
public:
    ShowPidCommand(const char *cmd_line);

    virtual ~ShowPidCommand() {}

    void execute() override;
};

class JobsList {
public:
    class JobEntry {

    public:
        int jobID;
        pid_t jobPID;
        std::string command;
        bool isStopped;
        JobEntry(int jobID, pid_t jobPID, std::string command, bool isStopped);
//            this->jobID = jobID;
//            this->jobPID = jobPID;
//            this->command = command;
//            this->isStopped = isStopped;
//        }
    };


public:
    std::vector<JobEntry> jobsList;

    int maxJobID = 0;

    int jobsCount;

    JobsList();

    ~JobsList();

    void addJob(Command *cmd, pid_t pid, bool isStopped = false);

    void printJobsList();

    void killAllJobs();

    void removeFinishedJobs();

    JobEntry *getJobById(int jobId);

    void removeJobById(int jobId);

    JobEntry *getLastJob(int *lastJobId);

    JobEntry *getLastStoppedJob(int *jobId);

};


class ChpromptCommand : public BuiltInCommand {
public:
    ChpromptCommand(const char *cmd_line);

    virtual ~ChpromptCommand() {}

    void execute() override;
};

class QuitCommand : public BuiltInCommand {

public:

    JobsList* jobs;
    QuitCommand(const char *cmd_line, JobsList *jobs);

    virtual ~QuitCommand() {}

    void execute() override;
};


class JobsCommand : public BuiltInCommand {
public:
    JobsList *jobs;
    JobsCommand(const char *cmd_line, JobsList *jobs);

    virtual ~JobsCommand() {}

    void execute() override;
};

class KillCommand : public BuiltInCommand {
    JobsList *jobs;
public:
    KillCommand(const char *cmd_line, JobsList *jobs);

    virtual ~KillCommand() {}

    void execute() override;
};

class ForegroundCommand : public BuiltInCommand {
    JobsList *jobs;
public:
    ForegroundCommand(const char *cmd_line, JobsList *jobs);

    virtual ~ForegroundCommand() {}

    void execute() override;
};

class ListDirCommand : public BuiltInCommand {
public:
    ListDirCommand(const char *cmd_line);

    virtual ~ListDirCommand() {}

    void execute() override;
};

class GetUserCommand : public BuiltInCommand {
public:
    GetUserCommand(const char *cmd_line);

    virtual ~GetUserCommand() {}

    void execute() override;
};

class aliasCommand : public BuiltInCommand {
public:
    aliasCommand(const char *cmd_line);

    virtual ~aliasCommand() {}

    void execute() override;
};

class unaliasCommand : public BuiltInCommand {
public:
    unaliasCommand(const char *cmd_line);

    virtual ~unaliasCommand() {}

    void execute() override;
};

class AliasEntry {

public:
    std::string aliasName;
    char *commandLine;
};


//public:

//};

class RedirectionCommand : public Command
{
public:
    std::string redirect_type;
    std::string file_name;
    std::string command;

    explicit RedirectionCommand(const char *cmd_line);

    virtual ~RedirectionCommand() {}

    void execute() override;
};

class PipeCommand : public Command
{

public:
    PipeCommand(const char *cmd_line);

    virtual ~PipeCommand() {}

    void execute() override;
};

class SmallShell {
private:
    SmallShell();

public:
    pid_t pid;
    pid_t currentProcess;
    pid_t watchProcess;
    std::string currentCmd;
    JobsList jobsList;
    int fgJobID;
    bool isFork;
    bool pipe;
    int fd;
    char *prev_directory;
    std::string smash_prompt;
    std::list<AliasEntry> aliasList;
    bool sigStop;
    //std::list<std::string> reservedCommands;

    Command *CreateCommand(const char *cmd_line);

    SmallShell(SmallShell const &) = delete; // disable copy ctor
    void operator=(SmallShell const &) = delete; // disable = operator
    static SmallShell &getInstance() // make SmallShell singleton
    {


        static SmallShell instance; // Guaranteed to be destroyed.
        // Instantiated on first use.


        return instance;
    }

    ~SmallShell();

    void executeCommand(const char *cmd_line);

};

#endif //SMASH_COMMAND_H_
