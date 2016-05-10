/**
 * xhyve-manager
 * a simple CLI utility to manage xhyve virtual machines.
 *
 * Usage: ./xhyve-manager \
 *   + {list} all available machines
 *   + {create,delete,start} <machine-name>
 *
 **/

// System
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <pwd.h>
#include <assert.h>

// Local
#include <xhyvectl/xhyvectl.h>
#include <ini/ini.h>

void print_usage(char **argv) {
  fprintf(stderr, "%s <command> <machine-name>\n", *argv);
}

int main(int argc, char **argv) {
  if (argc < 2) {
    print_usage(argv);
    exit(EXIT_FAILURE);
  }
  return 0;
}


