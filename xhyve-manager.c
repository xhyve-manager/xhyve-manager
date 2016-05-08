/**
 * xhyve-manager
 * a simple CLI utility to manage xhyve virtual machines.
 **/

// System
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

// Preprocessed
#define PROGRAM_EXEC 0
#define COMMAND 1
#define MACHINE_NAME 2

// Messages
#define MESSAGE_RUN_COMMAND "%s machine %s"

// Error Messages
#define ERROR_INVALID_COMMAND "%s is not a valid command."
#define ERROR_MACHINE_NOTFOUND "Could not find VM %s"

// Valid Commands
char *commands[] = {
  "list",
  "create",
  "delete",
  "start",
  NULL
};

// Functions
int is_valid_command(const char *command);
void list_machines();
void usage(const char *program_exec);
void run_command(const char *command, const char *machine_name);

int is_valid_command(const char *command) {
  int isValid = 0;
  char **temp = NULL;
  temp = commands;

  while (*temp) {
    if (strcmp(command,*temp) == 0) {
      isValid = 1;
      break;
    }
    temp++;
  }

  return isValid;
}

void list_machines() {
  fprintf(stdout, "Here be a list of machines:\n");
  fprintf(stdout, " - default\n");
  fprintf(stdout, " - different\n");
}

void usage(const char *program_exec) {
  fprintf(stderr, "Usage: %s <command> <virtual-machine-name> \n", program_exec);
}

void run_command(const char *command, const char *machine_name) {
  if (is_valid_command(command)) {
    fprintf(stdout, MESSAGE_RUN_COMMAND, command, machine_name);
    fprintf(stdout, "\n");
  } else {
    fprintf(stderr, ERROR_INVALID_COMMAND, command);
    fprintf(stdout, "\n");
  }
}

int main(int argc, char **argv) {
  if (argv[MACHINE_NAME] && argv[COMMAND]) {
    run_command(argv[COMMAND], argv[MACHINE_NAME]);
  } else if (!argv[MACHINE_NAME] && argv[COMMAND]) {
    list_machines();
  } else {
    usage(argv[PROGRAM_EXEC]);
    exit(EXIT_FAILURE);
  }
  exit(EXIT_SUCCESS);
}

