/**
 * xhyve-manager
 * a simple CLI utility to manage xhyve virtual machines.
 *
 * Usage: ./xhyve-manager \
 *   + {list} all available machines
 *   + {create,delete,start} <machine-name>
 *
 **/


/**
 * Lists existing virtual machines
 **/
void list_machines();

/**
 * Creates a new virtual machine
 * @param machine_name the name of the new machine
 **/
void create_machine(const char *machine_name);

/**
 * Delete virtual machine
 * @param machine_name the name of the new machine
 **/
void delete_machine(const char *machine_name);

/**
 * Start machine
 * @param machine_name the name of the new machine
 **/
void start_machine(const char *machine_name);
