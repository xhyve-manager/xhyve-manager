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

// Constants
#define DEFAULT_VM_DIR "/usr/local/Library/xhyve/machines"
#define DEFAULT_VM_EXT "xhyvm"

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

void load_config(xhyve_virtual_machine_t *machine, const char *name, const char *path) {
  char *config_path = NULL;
  const char *base_path = DEFAULT_VM_DIR;

  if (path) {
    config_path = strdup(path);
  } else {
    asprintf(&config_path, "%s/%s.%s/%s", base_path, name, DEFAULT_VM_EXT, "config.ini");
  }

  if (ini_parse(config_path, handler, machine) < 0) {
    fprintf(stderr, "Sorry, cannot load config.ini\n");
  } else {
    fprintf(stdout, "[type] %s\n[memory] %s\n[cpus] %s\n", machine->type, machine->memory, machine->cpus);
  }
}

void print_usage(void) {
  fprintf(stderr, "xhyve-manager <command> <machine-name> [machine-path]\n");
}

int main(int argc, char **argv) {
  if (argc < 2) {
    print_usage();
    exit(EXIT_FAILURE);
  }

  int opt;
  char *name = NULL;
  char *path = NULL;

  while ((opt = getopt(argc, argv, "n:p:")) != -1) {
    switch (opt) {
    case 'n':
      name = optarg;
      break;
    case 'p':
      path = optarg;
      break;
    default:
      print_usage();
      exit(EXIT_FAILURE);
    }
  }

  if (name || path) {
    xhyve_virtual_machine_t *machine = NULL;
    machine = malloc(sizeof(xhyve_virtual_machine_t));
    load_config(machine, name, path);

    exit(EXIT_SUCCESS);
  } else {
    print_usage();
    exit(EXIT_FAILURE);
  }
}


