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
#include <xhyve-manager/xhyve-manager.h>
#include <ini/ini.h>

static int handler(void* machine, const char* section, const char* key,
                   const char* value)
{
  xhyve_virtual_machine_t *pconfig = (xhyve_virtual_machine_t *)machine;

#define MATCH(s, k) strcmp(section, s) == 0 && strcmp(key, k) == 0
  if (MATCH("machine", "type")) {
    pconfig->type = strdup(value);
  } else if (MATCH("options", "memory")) {
    pconfig->memory = strdup(value);
  } else if (MATCH("options", "cpus")) {
    pconfig->cpus = strdup(value);
  } else {
    return 0;
  }

  return 1;
}

void print_usage(char **argv) {
  fprintf(stderr, "%s <command> <machine-name>\n", *argv);
}

int main(int argc, char **argv) {
  if (argc < 2) {
    print_usage(argv);
    exit(EXIT_FAILURE);
  }

  xhyve_virtual_machine_t *machine = NULL;
  machine = malloc(sizeof(xhyve_virtual_machine_t));

  if (ini_parse("config.ini", handler, machine) < 0) {
    fprintf(stderr, "Sorry\n");
  } else {
    fprintf(stdout, "[name] %s\n[memory] %s\n[cpus] %s\n", machine->type, machine->memory, machine->cpus);
  }

  return 0;
}


