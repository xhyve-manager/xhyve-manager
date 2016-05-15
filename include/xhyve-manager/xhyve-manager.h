/**
 * xhyve-manager
 * a simple CLI utility to manage xhyve virtual machines.
 *
 * Usage: ./xhyve-manager \
 *   + {list} all available machines
 *   + {create,delete,start} <machine-name>
 *
 **/

#ifndef __XHYVEMANAGER_H__
#define __XHYVEMANAGER_H__

typedef struct xhyve_virtual_machine {
  #define CFG(s, n, default) char *s##_##n;
  #include <xhyve-manager/config.def> 
} xhyve_virtual_machine_t;

void setup_host_machine(void);
void edit_machine_config(xhyve_virtual_machine_t *machine);
void print_machine_info(xhyve_virtual_machine_t *machine);
void start_machine(xhyve_virtual_machine_t *machine);
void create_machine(xhyve_virtual_machine_t *machine);

// Helpers
void extract_linux_boot_images(const char *path);
char* get_vdisk_path(char *vdisk_name);
void create_virtual_disk(char *path, int size);
char *get_machine_path(const char *machine_name);
char *get_config_path(const char *machine_name);
const char *get_homedir(void);
void initialize_machine_config(xhyve_virtual_machine_t *machine);
void load_machine_config(xhyve_virtual_machine_t *machine, const char *machine_name, int newFile);
void write_machine_config(xhyve_virtual_machine_t *machine, char *config_path);
void parse_args(xhyve_virtual_machine_t *machine, const char *command, const char *param);
int print_usage(void);
void form_config_string(char **ret, const char* fmt, ...);
void cleanup(void *ptr);

#endif
