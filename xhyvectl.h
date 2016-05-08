/**
 * xhyve-manager
 * a simple CLI utility to manage xhyve virtual machines.
 *
 * Usage: ./xhyve-manager \
 *   + {list} all available machines
 *   + {create,delete,start} <machine-name>
 *
 **/

// Get homedir
const char *homedir;

// Preprocessed
#define PROGRAM_EXEC 0
#define COMMAND 1
#define MACHINE_NAME 2

// Messages
#define MESSAGE_PARSE_COMMAND "%s machine %s with command_id %d"

// Error Messages
#define ERROR_INVALID_COMMAND "%s is not a valid command"
#define ERROR_MACHINE_NOTFOUND "Could not find VM %s"
#define ERROR_NEEDS_MACHINE "You need to specify a machine name"

// Defaults
#define DEFAULT_VM_DIRECTORY "xhyve.d/machines"
#define VM_EXT "xhyvm"

// Valid Commands
#define LIST 0
#define CREATE 1
#define DELETE 2
#define START 3
char *commands[] = {
  "list",
  "create",
  "delete",
  "start",
  NULL
};

// Function Declarations
char *get_homedir();
int get_command(const char *command);
void usage(const char *program_exec);
void invalid_command(const char *command, const char *error_message);
void parse_command(const char *command, const char *machine_name);
void run_command(const int command_id, const char *machine_name);
