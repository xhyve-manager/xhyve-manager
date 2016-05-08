
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

int main(int argc, char **argv) {
  program_path = argv[0];

  int opt;
  char *machine_name, *command;

  while ((opt = getopt(argc, argv, "n::")) != -1) {
    switch (opt) {
    case 'n':
      machine_name = optarg;
      command = argv[optind];
      break;
    default:
      usage();
    }
  }

  fprintf(stdout, "%s machine %s\n", command, machine_name);

  exit(EXIT_SUCCESS);
}

