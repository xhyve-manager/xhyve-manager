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
#include <stdarg.h>
#include <string.h>
#include <sys/stat.h>
#include <pwd.h>
#include <assert.h>
#include <uuid/uuid.h>

// Constants
#define DEFAULT_VM_DIR "xhyve VMs"
#define DEFAULT_VM_EXT "xhyvm"

// Macros
#define MATCH(s, n) strcmp(s, n) == 0

// Local
#include <xhyve-manager/xhyve-manager.h>
#include <ini/ini.h>

static char *program_exec;

static int handler(void* machine, const char* section, const char* name,
                   const char* value)
{
  xhyve_virtual_machine_t *pconfig = (xhyve_virtual_machine_t *)machine;

  if (0) ;
#define CFG(s, n, default) else if (strcmp(section, #s)==0 && \
                                    strcmp(name, #n)==0) pconfig->s##_##n = strdup(value);
#include <xhyve-manager/config.def>

  return 1;
}

char *get_machine_path(const char *machine_name)
{
  char *machine_path = NULL;
  asprintf(&machine_path, "%s/%s/%s.%s", get_homedir(), DEFAULT_VM_DIR, machine_name, DEFAULT_VM_EXT);
  return machine_path;
}

static char *get_firmware_type(xhyve_virtual_machine_t *machine)
{
  char *firmware = NULL;
  if (MATCH(machine->machine_type, "bsd"))
    firmware = "fbsd";
  else if (MATCH(machine->machine_type, "linux"))
    firmware = "kexec";
  else {
    fprintf(stderr, "Supported machine types: linux, bsd\n");
    exit(EXIT_FAILURE);
  }

  return firmware;
}

void form_config_string(char **ret, const char* fmt, ...)
{
  // From http://en.cppreference.com/w/c/variadic
  va_list args;
  va_start(args, fmt);
  asprintf(ret, "%s", "");

  const char *next;

  while (*fmt != '\0') {
    next = fmt + 1;

    if (*fmt == 's') {
      char *s = va_arg(args, char *);

      if (s) {
        asprintf(ret, "%s%s", *ret, s);
        if (*next != '\0') {
          asprintf(ret, "%s,", *ret);
        }
      }
    }
    ++fmt;
  }

  va_end(args);
}

void start_machine(xhyve_virtual_machine_t *machine)
{
  char *uuid = machine->machine_uuid;
  char *memory = machine->memory_size;
  char *cpus = machine->processor_cpus;
  char *firmware = get_firmware_type(machine);
  char *acpi = NULL;
  char *bridge = NULL;
  char *lpc = NULL;
  char *lpc_dev = NULL;
  char *networking = NULL;
  char *internal_storage = NULL;
  char *external_storage = NULL;

#define CFG(s, n, default) if (MATCH(#s, "boot")) form_config_string(&firmware, "ss", firmware, machine->s##_##n); \
  if (MATCH(#s, "bridge") && !(MATCH(machine->s##_##n, ""))) \
    form_config_string(&bridge, "ss", bridge, machine->s##_##n); \
  if (MATCH(#s, "lpc") && !(MATCH(machine->s##_##n, "")))               \
    form_config_string(&lpc, "ss", lpc, machine->s##_##n); \
  if (MATCH(#s, "lpc_dev") && !(MATCH(machine->s##_##n, "")))               \
    form_config_string(&lpc_dev, "ss", lpc_dev, machine->s##_##n); \
  if (MATCH(#s, "networking") && !(MATCH(machine->s##_##n, ""))) \
    form_config_string(&networking, "ss", networking, machine->s##_##n); \
  if (MATCH(#s, "internal_storage") && !(MATCH(machine->s##_##n, "")))  \
    form_config_string(&internal_storage, "ss", internal_storage, machine->s##_##n); \
  if (MATCH(#s, "external_storage") && !(MATCH(machine->external_storage_configinfo, "")) && !(MATCH(machine->s##_##n, ""))) \
    form_config_string(&external_storage, "ss", external_storage, machine->s##_##n); \
  if (MATCH(#s, "acpi") && MATCH(machine->acpi_enabled, "true")) form_config_string(&acpi, "s", "-A");

#include <xhyve-manager/config.def>

  char *exec_args[] = {
    "xhyve",
    "-f",
    firmware,
    "-U",
    uuid,
    "-m",
    memory,
    "-c",
    cpus,
    "-s",
    bridge,
    "-l",
    lpc_dev,
    "-s",
    lpc,
    "-s",
    networking,
    "-s",
    internal_storage,
    "",
    "",
    "",
    NULL
  };

  int argnum = 19;

  if (external_storage) exec_args[argnum++] = "-s", exec_args[argnum++] = external_storage;
  if (acpi) exec_args[argnum++] = acpi;

  int i;
  for (i = 0; i < argnum; i++){
    printf("%s\n", exec_args[i]);
  }

  char cwd[1024];
  chdir(get_machine_path(machine->machine_name));
  if (getcwd(cwd, sizeof(cwd)) != NULL)
    fprintf(stdout, "Current working dir: %s\n", cwd);

  execvp(*exec_args, exec_args);
}

void print_machine_info(xhyve_virtual_machine_t *machine)
{
#define CFG(s, n, default) printf("%s_%s = %s\n", #s, #n, machine->s##_##n);
#include <xhyve-manager/config.def>
}

char *get_config_path(const char *machine_name)
{
  char *config_path = NULL;
  asprintf(&config_path, "%s/config.ini", get_machine_path(machine_name));
  return config_path;
}

void initialize_machine_config(xhyve_virtual_machine_t *machine)
{
#define CFG(s, n, default) machine->s##_##n = strdup(default);
#include <xhyve-manager/config.def>
}

void load_machine_config(xhyve_virtual_machine_t *machine, const char *machine_name, int newFile)
{
  initialize_machine_config(machine);
  if (ini_parse(get_config_path(machine_name), handler, machine) < 0 && !newFile) {
    fprintf(stderr, "Missing or invalid machine config at %s\n", get_config_path(machine_name));
    exit(EXIT_FAILURE);
  }
}

void edit_machine_config(xhyve_virtual_machine_t *machine)
{
  char *editor = NULL;

  if ((editor = getenv("EDITOR")) == NULL) {
    editor = "nano";
  }

  fprintf(stdout, "\nEditing %s config with external editor: %s\n", machine->machine_name, editor);

  pid_t child;
  if ((child = fork()) == -1) {
    perror("fork");
  } else {
    if (child > 0) {
      int status;
      waitpid(child, &status, 0);
      if (WIFEXITED(status)) {
        fprintf(stdout, "\nEdited configuration for %s machine\n", machine->machine_name);
        print_machine_info(machine);
      }
    } else {
      execlp(editor, editor, get_config_path(machine->machine_name), (const char *) NULL);
    }
  }
}

static void get_input(char input[], char *message)
{
  fprintf(stdout, "%s: ", message);
  fgets(input, BUFSIZ, stdin);
  // http://stackoverflow.com/questions/2693776/removing-trailing-newline-character-from-fgets-input
  input[strcspn(input, "\r\n")] = 0;
  fprintf(stdout, "\n\n");
}

void create_machine(xhyve_virtual_machine_t *machine)
{
  initialize_machine_config(machine);
  int valid = 0;
  char input[BUFSIZ];

  fprintf(stdout, "So you want to create an xhyve virtual machine. Maybe I can help!\n");

  // Basic Machine Info
  get_input(input, "What would you like to name this machine?");
  machine->machine_name = strdup(input);

  while (!valid) {
    get_input(input, "Will this be a linux or bsd machine?");

    if (strcmp(input, "linux") == 0 || strcmp(input, "bsd") == 0) {
      valid = 1;
    } else {
      fprintf(stdout, "I'm sorry, '%s' is not a valid machine type.\n\n", input);
    }
  }
  machine->machine_type = strdup(input);

  // Internal Storage
  get_input(input, "Type in the full path to the virtual disk");
  machine->internal_storage_configinfo = strdup(input);

  // External Storage
  get_input(input, "Is there an ISO you would like to mount? Type in the full path below");
  machine->external_storage_configinfo = strdup(input);

  fprintf(stdout, "Below will be the configuration:\n");
  print_machine_info(machine);
}

void parse_args(xhyve_virtual_machine_t *machine, const char *command, const char *param)
{
  if (command && !param) {
    machine = malloc(sizeof(machine));
    initialize_machine_config(machine);
    if (MATCH(command, "create")) create_machine(machine);
    else print_usage();
  } else if (command && param) {

    if (!(MATCH(command, "create"))) {
      machine = malloc(sizeof(machine));
      initialize_machine_config(machine);
      load_machine_config(machine, param, 0);
    }

    if (MATCH(command, "create")) print_usage();

    if (MATCH(command, "edit")) edit_machine_config(machine);

    if (MATCH(command, "info")) print_machine_info(machine);

    if (MATCH(command, "start") && getuid() == 0) start_machine(machine);
    else fprintf(stderr, "You need to be root to start a VM"); exit(EXIT_FAILURE);

  } else {
    print_usage();
  }
}

int print_usage(void)
{
  fprintf(stderr, "Usage: %s <command> <machine-name>\n", program_exec);
  fprintf(stderr, "\tcommands:\n");
  fprintf(stderr, "\t  info: show info about VM\n");
  fprintf(stderr, "\t  start: start VM (needs root)\n");
  fprintf(stderr, "\t  edit: edit the configuration for VM\n");
  exit(EXIT_FAILURE);
}

// <slot,driver,configinfo> PCI slot config


const char *get_homedir(void)
{
  char *user = NULL;
  if (getuid() == 0) { // if root
    user = getenv("SUDO_USER");
  } else {
    user = getenv("USER");
  }

  struct passwd *pwd;
  pwd = getpwnam(user);
  return pwd->pw_dir;
}

int main(int argc, char **argv)
{
  program_exec = argv[0];
  if (argc < 1) {
    print_usage();
  }

  char *command = argv[1];
  char *machine_name = argv[2];
  xhyve_virtual_machine_t *machine = NULL;

  parse_args(machine, command, machine_name);
  exit(EXIT_SUCCESS);
}


