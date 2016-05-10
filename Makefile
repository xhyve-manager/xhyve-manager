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

.PHONY: install
install: all
	$(INSTALL) -C build/xhyve $(bindir)/$(binprefix)/xhyve

.PHONY: uninstall
uninstall:
	rm $(bindir)/$(binprefix)/xhyve

clean:
	@rm -rf build
