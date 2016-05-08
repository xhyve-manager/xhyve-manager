
// System
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

// Preprocessed
#define PROGRAM_EXEC 0
#define COMMAND 1
#define MACHINE_NAME 2

// Commands
#define LIST_COMMAND "list"

void list_machines() {
  fprintf(stdout, "Here be a list of machines:\n");
  fprintf(stdout, " - default\n");
  fprintf(stdout, " - different\n");
}

void usage(char **argv) {
  fprintf(stderr, "Usage: %s <virtual-machine-name> <command> \n", argv[PROGRAM_EXEC]);
}

void run_command(char *machine_name, char *command) {
  fprintf(stdout, "%s machine %s\n", machine_name, command);
}

int main(int argc, char **argv) {
  if (argv[MACHINE_NAME] && argv[COMMAND]) {
    run_command(argv[MACHINE_NAME], argv[COMMAND]);
  } else {
    list_machines();
    usage(argv);
    exit(EXIT_FAILURE);
  }
  exit(EXIT_SUCCESS);
}

