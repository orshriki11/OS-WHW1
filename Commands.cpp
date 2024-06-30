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

/////////////////////////////-------------Auxiliary functions-------------/////////////////////////////

//std::vector<std::string> init_args(const std::string &cmd_line, int &num_of_args) 
//{
//    std::vector<std::string> args;
//    std::istringstream iss(cmd_line);
//    std::string arg;
//
//    while (iss >> arg) {
//        args.push_back(arg);
//    }
//
//    if (args.size > COMMAND_MAX_ARGS){
//        args = nullptr;
//    }
//    num_of_args = args.size();
//    return args;
//}

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
//////////////////////////////-------------Built-in Commands-------------//////////////////////////////

/* Chprompt Command - chprompt command will allow the user to change the prompt displayed by 
                      the smash while waiting for the next command.*/

ChpromptCommand::ChpromptCommand(const char *cmd_line) : BuiltInCommand(cmd_line) {}

void ChpromptCommand::execute() 
{
    int num_of_args;
    char** args = initArgs(this->cmd_line, &num_of_args);
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
    char **args = init_args(this->cmd_line, &num_of_args);
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

JobsCommand::JobsCommand(const char *cmd_line) : BuiltInCommand(cmd_line) {}

void JobsCommand::execute() 
{
    SmallShell &shell = SmallShell::getInstance();

    shell.jobsList.removeFinishedJobs();

    shell.jobsList.printJobsList();
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
  string firstWord = cmd_s.substr(0, cmd_s.find_first_of(" \n"));

  if(firstWord.compare("chprompt") == 0) {
      return new ChpromptCommand(cmd_line);
  }
  else if (firstWord.compare("showpid") == 0) {
      return new ShowPidCommand(cmd_line);
  }
  else if (firstWord.compare("pwd") == 0) {
      return new GetCurrDirCommand(cmd_line);
  }
  //Need to add a proper last directory ref.
  else if (firstWord.compare("cd") == 0) {
      return new ChangeDirCommand(cmd_line, &currentDir);
  }
      //Need to add a proper last directory ref.
  else if (firstWord.compare("jobs") == 0) {
      return new JobsCommand(cmd_line);
  }
  else if (firstWord.compare("fg") == 0) {
      return new ForegroundCommand(cmd_line);
  }
  else if (firstWord.compare("quit") == 0) {
      return new QuitCommand(cmd_line);
  }
  else if (firstWord.compare("kill") == 0) {
      return new KillCommand(cmd_line);
  }
  else if (firstWord.compare("alias") == 0) {
      return new aliasCommand(cmd_line);
  }
  else if (firstWord.compare("unalias") == 0) {
      return new unaliasCommand(cmd_line);
  }
  else {
      return new ExternalCommand(cmd_line);
  }
    return nullptr;
}

void SmallShell::executeCommand(const char *cmd_line) {
    // TODO: Add your implementation here
    // for example:
    // Command* cmd = CreateCommand(cmd_line);
    // cmd->execute();
    // Please note that you must fork smash process for some commands (e.g., external commands....)
}