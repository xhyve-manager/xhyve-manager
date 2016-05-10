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
  char *type;
  char *memory;
  char *cpus;
} xhyve_virtual_machine_t;

void load_config(xhyve_virtual_machine_t *machine, const char *name);
void print_usage(char **argv);

#endif
