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
#include <err.h>

#include <assert.h>


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
#define DEFAULT_VM_DIRECTORY "/usr/local/Library/xhyve/machines"
#define VM_EXT "xhyvm"
#define DEFAULT_CONFIG_FILE "config.ini"

// Defaults for the virtual machine
#define DEFAULT_KERNEL  "machine/vmlinuz"
#define DEFAULT_INITRD  "machine/initrd.img"
#define DEFAULT_CMDLINE "earlyprintk=serial console=ttyS0 acpi=off root=/dev/centos/root ro"
#define DEFAULT_MEM     "1G"
#define DEFAULT_SMP     "2"
#define DEFAULT_NET     "virtio-net"
#define DEFAULT_IMG_HDD "machine/hdd.img"
#define DEFAULT_PCI_DEV "0:0,hostbridge -s 31,lpc"
#define DEFAULT_LPC_DEV "com1,stdio"

// Valid Commands
#define LIST 0
#define CREATE 1
#define DELETE 2
#define START 3
#define INFO 4

char *commands[] = {
  "list",
  "create",
  "delete",
  "start",
  "info",
  NULL
};

#define NUM_OPTIONS 9
char* machine_options[] = {
  "kernel",
  "initrd",
  "cmdline",
  "memory",
  "networking",
  "internal_storage",
  "external_storage",
  "pci_dev",
  "lpc_dev",
  NULL
};

typedef struct XVirtualMachineOptions {
  char *kernel;
  char *initrd;
  char *cmdline;
  char *memory;
  char *networking;
  char *internal_storage;
  char *external_storage;
  char *pci_dev;
  char *lpc_dev;
} xvirtual_machine_t;

// Globals
const char *homedir = NULL;
const char *program_exec = NULL;
xvirtual_machine_t *machine;

// Helper Functions
void get_machine_path(char *path, const char *machine_name);
int get_command(const char *command);
void usage();
void invalid_command(const char *command, const char *error_message);
void parse_command(const char *command, char *machine_name);
void run_command(const int command_id, char *machine_name);
void print_machine(xvirtual_machine_t *machine);
char *get_machine_option (xvirtual_machine_t *machine, int i);
void set_machine_option (xvirtual_machine_t *machine, int i, const char *value);
void cleanup();

// Tasks
void list_machines() {
  fprintf(stdout, "Here be a list of machines:\n");
  fprintf(stdout, " - default\n");
  fprintf(stdout, " - different\n");
}

void initialize_machine(xvirtual_machine_t *machine, char *machine_name, char path[]) {
  // Options
  machine->kernel           = DEFAULT_KERNEL;
  machine->initrd           = DEFAULT_INITRD;
  machine->cmdline          = DEFAULT_CMDLINE;
  machine->memory           = DEFAULT_MEM;
  machine->networking       = DEFAULT_NET;
  machine->internal_storage = DEFAULT_IMG_HDD;
  machine->external_storage = NULL;
  machine->pci_dev          = DEFAULT_PCI_DEV;
  machine->lpc_dev          = DEFAULT_LPC_DEV;
  print_machine(machine);
}

void create_machine(char *machine_name) {
  if (!machine_name) {
    fprintf(stderr, ERROR_NEEDS_MACHINE);
    fprintf(stderr, "\n");
  } else {
    fprintf(stdout, "This will create the machine %s\n", machine_name);

    // Create the machine_name.xhyvm directory
    char path[BUFSIZ];
    get_machine_path(path, machine_name);

    // Initialize
    machine = malloc(sizeof(xvirtual_machine_t));

    if (mkdir(path, 0700) == 0) {
      fprintf(stdout, "Creating %s.%s\n", machine_name,VM_EXT);
      initialize_machine(machine, machine_name, path);
    } else {
      perror("mkdir");
    }

  }
}

void delete_machine(const char *machine_name) {
}

// Read Xvirtual_Machine_T
static int handler(void* machine, const char* section, const char* name,
                   const char* value)
{
  xvirtual_machine_t* pconfig = (xvirtual_machine_t*)machine;

#define MATCH(s, n) strcmp(section, s) == 0 && strcmp(name, n) == 0
  int i;
  for (i = 0; i < NUM_OPTIONS; i++) {
    if (MATCH("options", machine_options[i]))
      set_machine_option(pconfig, i, value);
  }

  return 1;
}

void get_config_path(char *config_path, char *path) {
  sprintf(config_path, "%s/%s", path, DEFAULT_CONFIG_FILE);
}

void read_config(char *config_path) {
  machine = malloc(sizeof(xvirtual_machine_t));

  if (ini_parse(config_path, handler, machine) < 0) {
    fprintf(stderr, "Can't load %s\n", config_path);
  } else {
    print_machine(machine);
  }
}

void load_machine_info(const char *machine_name) {
  fprintf(stdout, "Configuration for %s\n", machine_name);
  char path[BUFSIZ];
  get_machine_path(path, machine_name);
  char config_path[BUFSIZ];
  get_config_path(config_path, path);
  read_config(config_path);
};

void start_machine(const char *machine_name) {
  load_machine_info(machine_name);
  pid_t child;

  int filedes[2];
  if (pipe(filedes) == -1) {
    perror("pipe");
    exit(1);
  }

  if ((child = fork()) == -1) {
    perror("fork");
  } else {
    if (!child) {

      // Pipe stdout
      dup2(filedes[1], STDOUT_FILENO);
      close(filedes[1]);
      close(filedes[0]);

      char path[BUFSIZ];
      get_machine_path(path, machine_name);
      if (chdir(path) < 0) {
        fprintf(stderr, "Could not go to %s", path);
        exit(EXIT_FAILURE);
      }

      // child
      char *args[] = {
        "xhyve",
        "-m",
        "1G",
        "-s",
        "0:0,hostbridge",
        "-s",
        "31,lpc",
        "-l",
        "com1,stdio",
        "-s",
        "2:0,virtio-net",
        "-s",
        "4,virtio-blk,machine/hdd.img",
        "-f",
        "kexec,machine/vmlinuz,machine/initrd.img,earlyprintk=serial",
        "console=ttyS0",
        "acpi=off",
        "root=/dev/centos/root",
        "ro",
        NULL
      };

      execv(*args, args);
      exit(EXIT_SUCCESS);
    } else {
      close(filedes[1]);
      wait(NULL);
    }
  }
}

// Main
int main(int argc, char **argv) {
  program_exec = argv[PROGRAM_EXEC];
  if (argv[COMMAND]) {
    parse_command(argv[COMMAND], argv[MACHINE_NAME]);
  } else {
    usage();
    exit(EXIT_FAILURE);
  }
  cleanup();
  exit(EXIT_SUCCESS);
}

//-------- Helper Functions -------------//

char *get_homedir() {
  char *hdir;
  // http://stackoverflow.com/questions/2910377/get-home-directory-in-linux-c
  struct passwd *pw = getpwuid(getuid());
  hdir = pw->pw_dir;
  return hdir;
}

void get_machine_path(char *path, const char *machine_name) {
  sprintf(path, "%s/%s.%s", DEFAULT_VM_DIRECTORY, machine_name, VM_EXT);
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
      usage();
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
  case INFO:
    load_machine_info(machine_name);
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

void print_machine(xvirtual_machine_t *machine) {
  printf("== MACHINE CONFIG ==\n");
  int i;
  for (i = 0; i < NUM_OPTIONS; i++)
    fprintf(stdout, "%s: %s\n", machine_options[i], get_machine_option(machine, i));
}

void cleanup() {
}

void set_machine_option (xvirtual_machine_t *machine, int i, const char *value) {
  switch(i) {
  case 0:  machine->kernel = strdup(value);
  case 1:  machine->initrd = strdup(value);
  case 2:  machine->cmdline = strdup(value);
  case 3:  machine->memory = strdup(value);
  case 4:  machine->networking = strdup(value);
  case 5:  machine->internal_storage = strdup(value);
  case 6:  machine->external_storage = strdup(value);
  case 7:  machine->pci_dev = strdup(value);
  case 8:  machine->lpc_dev = strdup(value);
  }
 }

char *get_machine_option (xvirtual_machine_t *machine, int i) {
  switch(i) {
  case 0: return machine->kernel;
  case 1: return machine->initrd;
  case 2: return machine->cmdline;
  case 3: return machine->memory;
  case 4: return machine->networking;
  case 5: return machine->internal_storage;
  case 6: return machine->external_storage;
  case 7: return machine->pci_dev;
  case 8: return machine->lpc_dev;
  }

  assert(0);
}
