GIT_VERSION := $(shell git describe --abbrev=6 --dirty --always --tags)

ifeq ($V, 1)
	VERBOSE =
else
	VERBOSE = @
endif

include config.mk

TARGET = xhyve-manager
XHYVEMANAGER_EXEC = build/$(TARGET)

VMM_SRC := \
	src/vmm/x86.c \
	src/vmm/vmm.c \
	src/vmm/vmm_host.c \
	src/vmm/vmm_mem.c \
	src/vmm/vmm_lapic.c \
	src/vmm/vmm_instruction_emul.c \
	src/vmm/vmm_ioport.c \
	src/vmm/vmm_callout.c \
	src/vmm/vmm_stat.c \
	src/vmm/vmm_util.c \
	src/vmm/vmm_api.c \
	src/vmm/intel/vmx.c \
	src/vmm/intel/vmx_msr.c \
	src/vmm/intel/vmcs.c \
	src/vmm/io/vatpic.c \
	src/vmm/io/vatpit.c \
	src/vmm/io/vhpet.c \
	src/vmm/io/vioapic.c \
	src/vmm/io/vlapic.c \
	src/vmm/io/vpmtmr.c \
	src/vmm/io/vrtc.c

XHYVE_SRC := \
	src/acpitbl.c \
	src/atkbdc.c \
	src/block_if.c \
	src/consport.c \
	src/dbgport.c \
	src/inout.c \
	src/ioapic.c \
	src/md5c.c \
	src/mem.c \
	src/mevent.c \
	src/mptbl.c \
	src/pci_ahci.c \
	src/pci_emul.c \
	src/pci_hostbridge.c \
	src/pci_irq.c \
	src/pci_lpc.c \
	src/pci_uart.c \
	src/pci_virtio_block.c \
	src/pci_virtio_net_tap.c \
	src/pci_virtio_net_vmnet.c \
	src/pci_virtio_rnd.c \
	src/pm.c \
	src/post.c \
	src/rtc.c \
	src/smbiostbl.c \
	src/task_switch.c \
	src/uart_emul.c \
	src/xhyve.c \
	src/virtio.c \
	src/xmsr.c

FIRMWARE_SRC := \
	src/firmware/kexec.c \
	src/firmware/fbsd.c

XHYVEMANAGER_SRC := \
  src/$(TARGET).c

INI_SRC := \
	src/ini/ini.c

SRC := \
	$(VMM_SRC) \
	$(XHYVE_SRC) \
	$(FIRMWARE_SRC) \
	$(XHYVEMANAGER_SRC) \
	$(INI_SRC)

OBJ := $(SRC:src/%.c=build/%.o)
DEP := $(OBJ:%.o=%.d)
INC := -Iinclude

CFLAGS += -DVERSION=\"$(GIT_VERSION)\"


INSTALL = /usr/bin/install

prefix = /usr/local
binprefix =
bindir = $(prefix)/bin
builddir = $(CURDIR)/build



all: $(XHYVEMANAGER_EXEC) | build

.PHONY: clean all
.SUFFIXES:

-include $(DEP)

build:
	@mkdir -p $(builddir)

build/%.o: src/%.c
	@echo cc $<
	@mkdir -p $(dir $@)
	$(VERBOSE) $(ENV) $(CC) $(CFLAGS) $(INC) $(DEF) -MMD -MT $@ -MF build/$*.d -o $@ -c $<

$(XHYVEMANAGER_EXEC).sym: $(OBJ)
	@echo ld $(notdir $@)
	$(VERBOSE) $(ENV) $(LD) $(LDFLAGS) -Xlinker $(XHYVEMANAGER_EXEC).lto.o -o $@ $(OBJ)
	@echo dsym $(notdir $(XHYVEMANAGER_EXEC).dSYM)
	$(VERBOSE) $(ENV) $(DSYM) $@ -o $(XHYVEMANAGER_EXEC).dSYM

$(XHYVEMANAGER_EXEC): $(XHYVEMANAGER_EXEC).sym
	@echo strip $(notdir $@)
	$(VERBOSE) $(ENV) $(STRIP) $(XHYVEMANAGER_EXEC).sym -o $@

.PHONY: install
install: all
	$(INSTALL) -C $(XHYVEMANAGER_EXEC) $(bindir)/$(binprefix)/$(TARGET)

.PHONY: uninstall
uninstall:
	rm $(bindir)/$(binprefix)/xhyve

.PHONY: clean
clean:
	@rm -rf build


TEST_INFO_0 = $(XHYVEMANAGER_EXEC) CentOS info

test: all test-info
	@echo "Tests done"

test-info:
	@echo "\033[33m\n--->\t$(TEST_INFO_0)\n\033[0m" && $(TEST_INFO_0)

