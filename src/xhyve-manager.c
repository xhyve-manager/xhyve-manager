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
#include <fcntl.h>
#include <pwd.h>
#include <assert.h>
#include <errno.h>
#include <uuid/uuid.h>

// Constants
#define DEFAULT_NUM_STARTERS 1
#define DEFAULT_VDISKS_DIR "VDisks"
#define DEFAULT_VM_DIR "Xhyve Virtual Machines"
#define DEFAULT_VM_EXT "xhyvm"
#define DEFAULT_SHARED "/usr/local/share/xhyve-manager"

// Macros
#define MATCH(s, n) strcmp(s, n) == 0

// Local
#include <xhyve/xhyve.h>
#include <xhyve-manager/xhyve-manager.h>
#include <ini/ini.h>

static char *program_exec;

static int handler(void* machine, const char* section, const char* name,
                   const char* value)
{
  xhyve_virtual_machine_t *pconfig = (xhyve_virtual_machine_t *)machine;

  if (0) ;
#define CFG(s, n, default) else if (MATCH(section, #s) && \
                                    MATCH(name, #n)) pconfig->s##_##n = strdup(value);
#include <xhyve-manager/config.def>

  return 1;
}

void setup_host_machine(void)
{
  if (getuid() != 0) {
    fprintf(stderr, "This commands need to be run as Root\n");
    exit(EXIT_FAILURE);
  }
  char path[BUFSIZ];
  sprintf(path, "%s/%s", get_homedir(), DEFAULT_VM_DIR);

  // Setup host NFS
  fprintf(stdout, "Setting up NFS on host machine with base IP 192.168.64.xx\n");
  FILE *exports = fopen("/etc/exports", "a");
  fprintf(exports, "# BEGIN XHYVE\n");
  fprintf(exports, "/Users -mapall=501 -network 192.168.64.0 -alldirs -mask 255.255.255.0");
  fprintf(exports, "# END XHYVE\n");
  fclose(exports);

  // Restart NFS daemon
  pid_t child;
  if ((child = fork()) == -1) {
    perror("fork");
  } else {
    if (!child) {
      fprintf(stdout, "Restarting nfsd to reload the NFS configuration\n");
      execlp("nfsd", "nfsd", "restart", (const char *) NULL);
    } else {
      wait(NULL);
      fprintf(stdout, "Done\n");
    }
  }
}

void cleanup(void *ptr)
{
  if (ptr != NULL)
    free(ptr);
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
  if (MATCH(machine->machine_type, "bsd")) {
    firmware = "fbsd";
  } else if (MATCH(machine->machine_type, "linux")) {
    firmware = "kexec";
  } else {
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
  if (MATCH(#s, "external_storage") && !(MATCH(machine->external_storage_configinfo, ""))) \
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

  if (external_storage) {
    exec_args[argnum++] = "-s";
    exec_args[argnum++] = external_storage;
  }

  if (acpi) exec_args[argnum++] = acpi;

  int i;
  for (i = 0; i < argnum; i++){
    printf("%s\n", exec_args[i]);
  }

  char cwd[1024];
  chdir(get_machine_path(machine->machine_name));
  if (getcwd(cwd, sizeof(cwd)) != NULL)
    fprintf(stdout, "Current working dir: %s\n", cwd);

  run_xhyve(argnum, exec_args);
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
        fflush(stdout);
        fprintf(stdout, "\nEdited configuration for %s machine\n", machine->machine_name);
        load_machine_config(machine, machine->machine_name, 0); // reload machine info
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

char* get_vdisk_path(char *vdisk_name)
{
  char *vdisk_path = NULL;
  asprintf(&vdisk_path, "%s/%s/%s.img", get_homedir(), DEFAULT_VDISKS_DIR, vdisk_name);
  return vdisk_path;
}

void create_virtual_disk(char *path, int size)
{
  fprintf(stdout, "A %dGB disk will be made\n", size);
  char ofpath[BUFSIZ];
  sprintf(ofpath, "of=%s", path);
  char countstr[BUFSIZ];
  sprintf(countstr, "count=%d", size);

  pid_t child;
  if ((child = fork()) == 0) {
    execl("/bin/dd","dd","if=/dev/zero","bs=1g",countstr,ofpath, (char *)0);
  } else {
    wait(NULL);
    fprintf(stdout, "Disk created at %s\n", path);
  }
}

void write_machine_config(xhyve_virtual_machine_t *machine, char *config_path)
{

  FILE *config_file = fopen(config_path, "w");

  char *section = "";

#define CFG(s, n, default) if (!(MATCH(section, #s))) { section = #s; fprintf(config_file, "\n[%s]\n", section); } \
  if (MATCH(section, #s)) { fprintf(config_file, "%s = %s\n", #n, machine->s##_##n); }
#include <xhyve-manager/config.def>

  fclose(config_file);
  fprintf(stdout, "Created %s for %s\n", config_path, machine->machine_name);
}

void extract_linux_boot_images(const char *path)
{
  char full_path[BUFSIZ];
  sprintf(full_path, "if=%s", path);

  pid_t creator, copier;

  if ((copier = fork()) == 0) {
    if ((creator = fork()) == 0) {
      execl("/bin/dd","dd","if=/dev/zero","bs=2k","count=1","of=/tmp/tmp.iso", (char *)0);
    } else {
      wait(NULL);
      int file = open("/tmp/tmp.iso",
                      O_CREAT | O_RDWR | O_APPEND,
                      S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH );
      if (file < 0)
        {
          fprintf(stderr, "open error: %d [%s]\n", errno, strerror(errno));
          exit(1);
        }
      dup2(file,1);
      close(file);
      execl("/bin/dd","dd",full_path,"bs=2k","skip=1",(char *)0);
    }
  } else {
    wait(NULL);
    fprintf(stdout, "Now, you need to mount /tmp/tmp.iso by running `open /tmp/tmp.iso`\n");
    fprintf(stdout, "This will mount the Linux live ISO into a disk your computer can read.\n");
    fprintf(stdout, "Example:\n");
    fprintf(stdout, "\t$ xhyve-manager extract ~/Downloads/archlinux-2016.05.01-dual.iso\n");
    fprintf(stdout, "\t$ open /tmp/tmp.iso\n");
    fprintf(stdout, "\t$ cp /Volumes/ARCH_201605/boot/x86_64/{vmlinuz,archiso.img} ~/.xhyve.d/machines/Arch.xhyvm\n");
  }
}

void create_machine(xhyve_virtual_machine_t *machine)
{
  initialize_machine_config(machine);
  int valid = 0;
  char input[BUFSIZ];

  fprintf(stdout, "So you want to create an xhyve virtual machine. Maybe I can help!\n");

  // Basic Machine Info
  while (!valid) {
    get_input(input, "What would you like to name this machine?");
    if (mkdir(get_machine_path(input), 0755) == -1) {
        fprintf(stderr, "%s\n", strerror(errno));
    } else {
      machine->machine_name = strdup(input);
      valid = 1;
      break;
    }
  }

  // For pre-built starters
  get_input(input, "Do you want to use a starter kit? [y]/n");
  if (!(MATCH(input, "n"))) {
    char **starter = { 0 };
    starter = calloc(DEFAULT_NUM_STARTERS, 0);
    int i = 0;
    fprintf(stdout, "Valid starter kits:\n");
#define SK(n, v) i++; \
    fprintf(stdout, "%d %s %s\n", i, #n, #v); \
    asprintf(&starter[i], "%s/%s", #n, #v);
#include <xhyve-manager/starter_kits.def>
    get_input(input, "Type in a number corresponding to the starter kit (e.g. 1 for ubuntu)");

    int idx = atoi(input);
    asprintf(&machine->boot_kernel, "%s/%s/%s", DEFAULT_SHARED, starter[idx], "vmlinuz");
    asprintf(&machine->boot_initrd, "%s/%s/%s", DEFAULT_SHARED, starter[idx], "initrd.gz");
    machine->machine_type = "linux";
  } else {
    valid = 0;
    while (!valid) {
      fprintf(stdout, "Will this be a linux or bsd machine?");
      get_input(input, "linux [bsd]");

      if (MATCH(input, "linux")) {
        asprintf(&machine->boot_kernel, "%s/%s", DEFAULT_SHARED, "vmlinuz");
        asprintf(&machine->boot_initrd, "%s/%s", DEFAULT_SHARED, "initrd.gz");
        machine->machine_type = "linux";
      } else {
        machine->boot_options = "";
        machine->boot_initrd = "";
        asprintf(&machine->boot_kernel, "%s/%s", DEFAULT_SHARED, "userboot.so");
        machine->machine_type = strdup("bsd");
        machine->acpi_enabled = strdup("true");
      }
      valid = 1;
    }

  }

  // Generate UUID
  fprintf(stdout, "Generating a UUID\n");
  uuid_t uuid;
  uuid_generate(uuid);
  uuid_string_t uuid_str;
  uuid_unparse(uuid, uuid_str);
  machine->machine_uuid = strdup(uuid_str);
  fprintf(stdout, "The UUID of the machine will be %s\n", machine->machine_uuid);

  // Internal Storage
  valid = 0;
  while (!valid) {
    get_input(input, "Is there an existing virtual disk you would like to use? y/[n]");
    if (MATCH(input, "y")) {
      get_input(input, "Please type in the full path to the virtual disk: (ex. /Users/tris/VDisks/dauntless.img)");
      machine->internal_storage_configinfo = strdup(input);
      valid = 1;
    } else {
      get_input(input, "I can create a virtual disk for you! How much space should it use in GBs? (ex. 5 for 5GB)");
      char *vdisk_path = get_vdisk_path(uuid_str);
      create_virtual_disk(vdisk_path, atoi(input));
      machine->internal_storage_configinfo = strdup(vdisk_path);
      valid = 1;
      break;
    }
  }

  // External Storage
  get_input(input, "Is there an ISO, i.e. a CD image you would like to mount? [y]/n");
  if (!(MATCH(input, "n"))) {
    get_input(input, "What is the path to the CD image? (Type in the FULL absolute path.)");
    machine->external_storage_configinfo = strdup(input);
    if (MATCH(machine->machine_type, "bsd"))
      machine->boot_initrd = strdup(machine->external_storage_configinfo);
  }

  char *config_path = get_config_path(machine->machine_name);
  write_machine_config(machine, config_path);

  fprintf(stdout, "Below is the configuration.\n");
  print_machine_info(machine);
  get_input(input, "Would you like to edit? y/[n]");
  if (MATCH(input, "y"))
    edit_machine_config(machine);
  else
    fprintf(stdout, "Now you can try starting the VM with `%s start %s`", program_exec, machine->machine_name);

  fflush(stdout);
}

void parse_args(xhyve_virtual_machine_t *machine, const char *command, const char *param)
{
  if (command && !param) {
    machine = malloc(sizeof(machine));
    initialize_machine_config(machine);
    if (MATCH(command, "create")) create_machine(machine);
    else if (MATCH(command, "setup")) setup_host_machine();
    else print_usage();
  } else if (command && param) {
    if (MATCH(command, "extract"))
      extract_linux_boot_images(param);

    if (!(MATCH(command, "create")) && !(MATCH(command, "extract"))) {
      machine = malloc(sizeof(machine));
      initialize_machine_config(machine);
      load_machine_config(machine, param, 0);
    }

    if (MATCH(command, "create"))
      print_usage();
    else if (MATCH(command, "edit"))
      edit_machine_config(machine);
    else if (MATCH(command, "info"))
      print_machine_info(machine);
    else if (MATCH(command, "start")) {
      if (getuid() == 0) 
        start_machine(machine); 
      else
        fprintf(stderr, "You need to be Root to start a VM"); exit(EXIT_FAILURE);
    }
  } else {
    print_usage();
  }

  cleanup(machine);
}

int print_usage(void)
{
  fprintf(stderr, "Usage: %s <command> <machine-name>\n", program_exec);
  fprintf(stderr, "\tcommands:\n");
  fprintf(stderr, "\t  info: show info about VM\n");
  fprintf(stderr, "\t  start: start VM (needs root)\n");
  fprintf(stderr, "\t  edit: edit the configuration for VM\n");
  fprintf(stderr, "\t  create: create a VM\n");
  fprintf(stderr, "\t  extract: extract the needed boot images for Linux vms\n");
  fprintf(stderr, "\t  setup: setup host machine for NFS\n");
  exit(EXIT_FAILURE);
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


