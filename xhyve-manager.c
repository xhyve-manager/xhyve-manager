
// System
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

// Defaults
#define DEFAULT_MACHINE "xhyve_default"

// Global
char *program_path;

void usage() {
  fprintf(stderr, "Usage: %s -n <virtual-machine-name> <command> \n", program_path);
  exit(EXIT_FAILURE);
}

void list_machines() {
  fprintf(stdout, "Here be a list of machines:\n");
  fprintf(stdout, " - default\n");
  fprintf(stdout, " - different\n");
}

int main(int argc, char **argv) {
  program_path = argv[0];

  int opt;
  char *machine_name, *command;
  int listMachines = 0;

  while ((opt = getopt(argc, argv, "ln::")) != -1) {
    switch (opt) {
    case 'l':
      listMachines = 1;
      break;
    case 'n':
      machine_name = optarg;
      command = argv[optind];
      break;
    }
  }

  if (machine_name && command && !listMachines) {
    fprintf(stdout, "%s machine %s\n", command, machine_name);
  } else if (listMachines) {
    list_machines();
  } else {
    usage();
  }

  exit(EXIT_SUCCESS);
}

