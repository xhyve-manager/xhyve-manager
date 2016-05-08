
// System
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

// Defaults
#define DEFAULT_MACHINE "xhyve_default"

int main(int argc, char **argv) {
  int opt, worker_threads;
  char *machine_name;
  machine_name = DEFAULT_MACHINE;

  while ((opt = getopt(argc, argv, "n:")) != -1) {
    switch (opt) {
    case 'n':
      machine_name = optarg;
      break;
    default:
      fprintf(stderr, "Usage: %s [-n <virtual-machine-name>] \n", argv[0]);
      exit(EXIT_FAILURE);
    }
  }

  fprintf(stdout, "machine_name: %s, optind: %d\n", machine_name, optind);

  exit(EXIT_SUCCESS);
}

