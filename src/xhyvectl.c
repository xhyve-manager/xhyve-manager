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

// Local
#include "xhyvectl.h"
#include "ini.h"

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

// Defaults for the virtual machine
#define DEFAULT_MEM     "-m 1G"
#define DEFAULT_SMP     "-c 2"
#define DEFAULT_NET     "-s 2:0,virtio-net"
#define DEFAULT_IMG_HDD "-s 4,virtio-blk,centos/hdd.img"
#define DEFAULT_PCI_DEV "-s 0:0,hostbridge -s 31,lpc"
#define DEFAULT_LPC_DEV "-l com1,stdio"

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

typedef struct XVirtualMachineOptions {
  char *memory;
  char *networking;
  char *internal_storage;
  char *external_storage;
  char *pci_dev;
  char *lpc_dev;
} xvirtual_machine_options_t;

typedef struct XVirtualMachine {
  char *name;
  char *path;
  xvirtual_machine_options_t *machine_options;
} xvirtual_machine_t;

// Globals
const char *homedir = NULL;
xvirtual_machine_t *machine = NULL;
const char *program_exec = NULL;

// Helper Functions
char *get_homedir();
int get_command(const char *command);
void usage();
void invalid_command(const char *command, const char *error_message);
void parse_command(const char *command, char *machine_name);
void run_command(const int command_id, char *machine_name);
void cleanup();

// Tasks
void list_machines() {
  fprintf(stdout, "Here be a list of machines:\n");
  fprintf(stdout, " - default\n");
  fprintf(stdout, " - different\n");
}

xvirtual_machine_t *initialize_machine(char *machine_name, char *path) {
  xvirtual_machine_t *machine = malloc(sizeof(xvirtual_machine_t));
  machine->name = machine_name;
  machine->path = path;

  xvirtual_machine_options_t *machine_options = malloc(sizeof(machine_options));
  machine_options->memory           = DEFAULT_MEM;
  machine_options->networking       = DEFAULT_NET;
  machine_options->internal_storage = DEFAULT_IMG_HDD;
  machine_options->external_storage = NULL;
  machine_options->pci_dev          = DEFAULT_PCI_DEV;
  machine_options->lpc_dev          = DEFAULT_LPC_DEV;

  return machine;
}

void create_machine(char *machine_name) {
  if (!machine_name) {
    fprintf(stderr, ERROR_NEEDS_MACHINE);
    fprintf(stderr, "\n");
  } else {
    fprintf(stdout, "This will create the machine %s\n", machine_name);

    // Create the machine_name.xhyvm directory
    char path[BUFSIZ];
    sprintf(path, "%s/.%s/%s.%s", homedir, DEFAULT_VM_DIRECTORY, machine_name, VM_EXT);
    fprintf(stdout, "Creating %s.%s\n", machine_name,VM_EXT);
    if (mkdir(path, 0700) == 0) {
      machine = malloc(sizeof(xvirtual_machine_t));
      machine = initialize_machine(machine_name, path);
      fprintf(stdout,
              "Machine: %s %s",
              machine->name, machine->path);
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
  program_exec = argv[PROGRAM_EXEC];
  if (argv[COMMAND]) {
    homedir = get_homedir();
    parse_command(argv[COMMAND], argv[MACHINE_NAME]);
  } else {
    usage();
    exit(EXIT_FAILURE);
  }
  cleanup();
  exit(EXIT_SUCCESS);
}

char *get_homedir() {
  char *hdir;
  // http://stackoverflow.com/questions/2910377/get-home-directory-in-linux-c
  struct passwd *pw = getpwuid(getuid());
  hdir = pw->pw_dir;
  return hdir;
}

void parse_command(const char *command, char *machine_name) {
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

void run_command(const int command_id, char *machine_name) {
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
  usage();
}

void usage() {
  fprintf(stderr, "Usage: %s <command> <virtual-machine-name> \n", program_exec);
}

void cleanup() {
  if (machine) free(machine);
}
