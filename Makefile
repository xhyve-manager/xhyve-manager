GIT_VERSION := $(shell git describe --abbrev=6 --dirty --always --tags)

ifeq ($V, 1)
	VERBOSE =
else
	VERBOSE = 
endif

include config.mk

TARGET = xhyve-manager
XHYVEMANAGER_EXEC = build/$(TARGET)

INI_SRC := \
	src/ini/ini.c

XHYVEMANAGER_SRC := \
  src/$(TARGET).c

SRC := \
	$(INI_SRC) \
	$(XHYVEMANAGER_SRC)

OBJ := $(SRC:src/%.c=build/%.o)
DEP := $(OBJ:%.o=%.d)
INC := -Iinclude

CFLAGS += -DVERSION=\"$(GIT_VERSION)\"

INSTALL = /usr/bin/install

prefix = /usr/local
binprefix =
bindir = $(prefix)/bin
builddir = $(CURDIR)/build
shareddir = share/$(TARGET)

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
install: $(XHYVEMANAGER_EXEC) 
	$(INSTALL) -C $(XHYVEMANAGER_EXEC) $(bindir)/$(binprefix)/$(TARGET)
	cp -R $(shareddir) $(prefix)/share/$(TARGET)

.PHONY: uninstall
uninstall:
	rm $(bindir)/$(binprefix)/$(TARGET)
	rm -r $(prefix)/share/$(TARGET)

clean:
	rm -rf build

test: all test-info
	@echo "\nTests have been completed"

test-info:
	$(XHYVEMANAGER_EXEC) info CentOS
	$(XHYVEMANAGER_EXEC) info Ubuntu

