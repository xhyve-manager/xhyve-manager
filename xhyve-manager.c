/**
 * xhyve-manager
 * a simple CLI utility to manage xhyve virtual machines.
 **/

// System
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <pwd.h>

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

// Tasks
void list_machines() {
  fprintf(stdout, "Here be a list of machines:\n");
  fprintf(stdout, " - default\n");
  fprintf(stdout, " - different\n");
}

void create_machine(const char *machine_name) {
  if (!machine_name) {
    fprintf(stderr, ERROR_NEEDS_MACHINE);
    fprintf(stderr, "\n");
  } else {
    fprintf(stdout, "This will create the machine %s\n", machine_name);
    char path[BUFSIZ];
    sprintf(path, "%s/.%s/%s.%s", homedir, DEFAULT_VM_DIRECTORY, machine_name, VM_EXT);
    fprintf(stdout, "Creating %s.%s\n", machine_name,VM_EXT);
    if (mkdir(path, 0700) == 0) {
      fprintf(stdout, "Successfully initialized machine at %s\n", path);
    } else {
      perror("mkdir");
    }
  }
}

void delete_machine(const char *machine_name) {
}

void start_machine(const char *machine_name) {
}

// Main
int main(int argc, char **argv) {
  if (argv[COMMAND]) {
    homedir = get_homedir();
    parse_command(argv[COMMAND], argv[MACHINE_NAME]);
  } else {
    usage(argv[PROGRAM_EXEC]);
    exit(EXIT_FAILURE);
  }
  exit(EXIT_SUCCESS);
}

char *get_homedir() {
  char *hdir;
  // http://stackoverflow.com/questions/2910377/get-home-directory-in-linux-c
  struct passwd *pw = getpwuid(getuid());
  hdir = pw->pw_dir;
  return hdir;
}

void parse_command(const char *command, const char *machine_name) {
  int command_id;
  if ((command_id = get_command(command)) != -1) {
    run_command(command_id, machine_name);
  } else {
    invalid_command(command, ERROR_INVALID_COMMAND);
  }
}

int get_command(const char *command) {
  int command_id = -1, cid = 0;
  char **temp = NULL;
  temp = commands;

  while (*temp) {
    if (strcmp(command,*temp) == 0) {
      command_id = cid;
      break;
    }
    cid++;
    temp++;
  }

  return command_id;
}

void run_command(const int command_id, const char *machine_name) {
  switch (command_id) {
  case LIST:
    if (!machine_name) list_machines();
    else {
      fprintf(stderr, "list does not take any additional parameters\n");
      exit(EXIT_FAILURE);
    }
    break;
  case START:
    start_machine(machine_name);
    break;
  case CREATE:
    create_machine(machine_name);
    break;
  case DELETE:
    delete_machine(machine_name);
    break;
  }
}

void invalid_command(const char *command, const char *error_message) {
  fprintf(stderr, error_message, command);
  fprintf(stderr, "\n");
}

void usage(const char *program_exec) {
  fprintf(stderr, "Usage: %s <command> <virtual-machine-name> \n", program_exec);
}
