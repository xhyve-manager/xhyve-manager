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

// xhyve
#include <xhyve/xhyve.h>

// Constants
#define DEFAULT_VM_DIR "xhyve VMs"
#define DEFAULT_VM_EXT "xhyvm"

// Local
#include <xhyve-manager/xhyve-manager.h>
#include <ini/ini.h>

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

int start_machine(xhyve_virtual_machine_t *machine)
{
  char *pci_dev = NULL;
  char *pci_lpc = NULL;
  char *lpc_dev = NULL;
  char *net = NULL;
  char *img_hdd = NULL;
  char *firmware = NULL;

  form_config_string(&pci_dev, "ss", machine->bridge_slot, machine->bridge_driver);
  form_config_string(&pci_lpc, "ss", machine->lpc_slot, machine->lpc_driver);
  form_config_string(&lpc_dev, "s", machine->lpc_configinfo);
  form_config_string(&net, "ss", machine->networking_slot, machine->networking_driver);
  form_config_string(&img_hdd, "sss", machine->internal_storage_slot, machine->internal_storage_driver, machine->internal_storage_configinfo);

  char *machine_path = NULL;
  asprintf(&machine_path, "%s/%s.%s", DEFAULT_VM_DIR, machine->machine_name, DEFAULT_VM_EXT);
  chdir(machine_path);
  form_config_string(&firmware, "ssss", "kexec", machine->boot_kernel, machine->boot_initrd, machine->boot_options);

  char *args[] = {
    "xhyve",
    "-U",
    machine->machine_uuid,
    "-m",
    machine->memory_size,
    "-c",
    machine->processor_cpus,
    "-s",
    pci_dev,
    "-s",
    pci_lpc,
    "-l",
    lpc_dev,
    "-s",
    net,
    "-s",
    img_hdd,
    "-f",
    firmware,
    NULL
  };

  int argnum = 0;
  char **ptr = NULL;
  ptr = args;

  while (*ptr) {
    ++argnum;
    ++ptr;
  }

  return xhyve_entrypoint(argnum, args);
}

void print_machine_info(xhyve_virtual_machine_t *machine)
{
#define CFG(s, n, default) printf("%s_%s = %s\n", #s, #n, machine->s##_##n);
#include <xhyve-manager/config.def>
}

char *get_config_path(const char *name, const char *path)
{
  char *config_path = NULL;
  char *machine_path = NULL;

  if (path) {
    machine_path = strdup(path);
  } else {
    asprintf(&machine_path, "%s/%s.%s", DEFAULT_VM_DIR, name, DEFAULT_VM_EXT);
  }

  asprintf(&config_path, "%s/config.ini", machine_path);
  free(machine_path);

  return config_path;
}

void load_config(xhyve_virtual_machine_t *machine, const char *config_path)
{
  if (ini_parse(config_path, handler, machine) < 0) {
    fprintf(stderr, "Cannot load machine\n");
  }
}

void parse_args(const char *command, const char *param, xhyve_virtual_machine_t *machine)
{
  if (command) {
    machine = malloc(sizeof(xhyve_virtual_machine_t));
    load_config(machine, param);

    if (strcmp(command, "info") == 0) {
      print_machine_info(machine);
    } else if (strcmp(command, "start") == 0) {
      start_machine(machine);
    }
  } else {
    print_usage();
  }
}

int print_usage(void)
{
  fprintf(stderr, "Usage: xhyve-manager [-np <machine name or path>] <command>\n");
  fprintf(stderr, "\t-n: specify name of machine in XHYVMS directory\n");
  fprintf(stderr, "\t-p: specify path to xhyvm\n");
  fprintf(stderr, "\tcommands:\n");
  fprintf(stderr, "\t  info: show info about machine\n");
  fprintf(stderr, "\t  start: start a vm (needs root)\n");
  fprintf(stderr, "\t  create: create a VM\n");
  exit(EXIT_FAILURE);
}

// <slot,driver,configinfo> PCI slot config

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
      asprintf(ret, "%s%s", *ret, s);
      if (*next != '\0')
        asprintf(ret, "%s,", *ret);
    }
    ++fmt;
  }

  va_end(args);
}

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
  printf("HOMEDIR: %s\n", get_homedir());
  if (argc < 2) {
    print_usage();
  }

  int opt;
  char *command = NULL;
  char *name = NULL;
  char *path = NULL;
  char *param = NULL;
  xhyve_virtual_machine_t *machine = NULL;

  while ((opt = getopt(argc, argv, "n::p::")) != -1) {
    switch (opt) {
    case 'n':
      name = optarg;
      command = argv[optind];
      break;
    case 'p':
      path = optarg;
      command = argv[optind];
      break;
    default:
      print_usage();
    }
  }

  if (name || path) {
    if (name && path) {
      print_usage();
    }

    param = get_config_path(name, path);
    parse_args(command, param, machine);

    if (machine) free(machine);
    if (param) free(param);

    exit(EXIT_SUCCESS);
  } else {
    print_usage();
  }
}


