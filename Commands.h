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
// TODO: Add your data members
protected:
    const char* cmd_line;
public:
    Command(const char *cmd_line);

    virtual ~Command();

    virtual void execute() = 0;
    //virtual void prepare();
    //virtual void cleanup();
    std::string getCmdLine() { return std::string(cmd_line); }
    // TODO: Add your extra methods if needed
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

class PipeCommand : public Command {
    // TODO: Add your data members
public:
    PipeCommand(const char *cmd_line);

    virtual ~PipeCommand() {}

    void execute() override;
};

class WatchCommand : public Command {
    // TODO: Add your data members
public:
    WatchCommand(const char *cmd_line);

    virtual ~WatchCommand() {}

    void execute() override;
};

class RedirectionCommand : public Command {
    // TODO: Add your data members
public:
    explicit RedirectionCommand(const char *cmd_line);

    virtual ~RedirectionCommand() {}

    void execute() override;
};

class ChangeDirCommand : public BuiltInCommand {
// TODO: Add your data members public:
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
        // TODO: Add your data members
    public:
        int jobID;
        pid_t jobPID;
        std::string command;
        bool isStopped;
        JobEntry(int jobID, pid_t jobPID, std::string command, bool isStopped) {
            this->jobID = jobID;
            this->jobPID = jobPID;
            this->command = command;
            this->isStopped = isStopped;
        }
    };
    // TODO: Add your data members

public:
    std::vector<JobEntry> jobsList;

    int maxJobID = 0;

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
    // TODO: Add extra methods or modify exisitng ones as needed
};


class QuitCommand : public BuiltInCommand {
// TODO: Add your data members public:
    //JobsList jobsList;
public:
    QuitCommand(const char *cmd_line, JobsList *jobs);

    virtual ~QuitCommand() {}

    void execute() override;
};


class JobsCommand : public BuiltInCommand {
    // TODO: Add your data members
public:
    JobsList *jobs;
    JobsCommand(const char *cmd_line, JobsList *jobs);

    virtual ~JobsCommand() {}

    void execute() override;
};

class KillCommand : public BuiltInCommand {
    // TODO: Add your data members
public:
    KillCommand(const char *cmd_line, JobsList *jobs);

    virtual ~KillCommand() {}

    void execute() override;
};

class ForegroundCommand : public BuiltInCommand {
    // TODO: Add your data members
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

//class AliasList {
//public:
    class AliasEntry {
        // TODO: Add your data members
    public:
        std::string aliasName;
        char *commandLine;
    };
    // TODO: Add your data members

//public:

//};

class RedirectionCommand : public Command
{
    // TODO: Add your data members
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
    // TODO: Add your data members
    SmallShell();

public:
    static pid_t pid;
    pid_t currentProcess;
    std::string currentCmd;
    static JobsList jobsList;
    int fgJobID;
    bool isFork;
    bool pipe;
    std::string smash_prompt = "smash";
    std::list<AliasEntry> aliasList;
    std::list<std::string> reservedCommands{"chprompt","showpid","pwd","cd","jobs","fg","quit","kill","alias","unalias",
                                       "listdir", "getuser", "watch"};

    Command *CreateCommand(const char *cmd_line);

    SmallShell(SmallShell const &) = delete; // disable copy ctor
    void operator=(SmallShell const &) = delete; // disable = operator
    static SmallShell &getInstance() // make SmallShell singleton
    {
        /*reservedCommands.pushback("chprompt");
        reservedCommands.pushback("showpid");
        reservedCommands.pushback("pwd");
        reservedCommands.pushback("cd");
        reservedCommands.pushback("jobs");
        reservedCommands.pushback("fg");
        reservedCommands.pushback("quit");
        reservedCommands.pushback("kill");
        reservedCommands.pushback("alias");
        reservedCommands.pushback("unalias");
        reservedCommands.pushback("listdir");
        reservedCommands.pushback("getuser");
        reservedCommands.pushback("watch");*/


        static SmallShell instance; // Guaranteed to be destroyed.
        // Instantiated on first use.


        return instance;
    }

    ~SmallShell();

    void executeCommand(const char *cmd_line);
    // TODO: add extra methods as needed

};

#endif //SMASH_COMMAND_H_
