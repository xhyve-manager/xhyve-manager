GIT_VERSION := $(shell git describe --abbrev=6 --dirty --always --tags)

ifeq ($V, 1)
	VERBOSE =
else
	VERBOSE = @
endif

include config.mk

TARGET = xhyve-manager
XHYVE_EXEC = build/$(TARGET)

XHYVE_SRC := \
  src/$(TARGET).c

INI_SRC := \
	src/ini/ini.c

SRC := \
	$(XHYVE_SRC) \
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



all: $(XHYVE_EXEC) | build

.PHONY: clean all
.SUFFIXES:

-include $(DEP)

build:
	@mkdir -p $(builddir)

build/%.o: src/%.c
	@echo cc $<
	@mkdir -p $(dir $@)
	$(VERBOSE) $(ENV) $(CC) $(CFLAGS) $(INC) $(DEF) -MMD -MT $@ -MF build/$*.d -o $@ -c $<

$(XHYVE_EXEC).sym: $(OBJ)
	@echo ld $(notdir $@)
	$(VERBOSE) $(ENV) $(LD) $(LDFLAGS) -Xlinker $(XHYVE_EXEC).lto.o -o $@ $(OBJ)
	@echo dsym $(notdir $(XHYVE_EXEC).dSYM)
	$(VERBOSE) $(ENV) $(DSYM) $@ -o $(XHYVE_EXEC).dSYM

$(XHYVE_EXEC): $(XHYVE_EXEC).sym
	@echo strip $(notdir $@)
	$(VERBOSE) $(ENV) $(STRIP) $(XHYVE_EXEC).sym -o $@

.PHONY: clean
clean:
	@rm -rf build


UUIDGEN = /usr/bin/uuidgen
UUID = `UUIDGEN`
TEST_INFO_0 = $(XHYVE_EXEC) -n CentOS info
TEST_INFO_1 = $(XHYVE_EXEC) -p /usr/local/Library/xhyve/machines/CentOS.xhyvm info

test: clean all test-info test-create
	@echo "Tests done"

test-info:
	@echo "\033[33m\n--->\t$(TEST_INFO_0)\n\033[0m" && $(TEST_INFO_0)
	@echo "\033[33m\n--->\t$(TEST_INFO_1)\n\033[0m" && $(TEST_INFO_1)

test-create:
	@echo "\033[33m\n--->\tTest VM creation\n\033[0m"
	@$(XHYVE_EXEC) -n $(UUID) create
