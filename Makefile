IDIR = include
CC=clang
CFLAGS=-I$(IDIR)

ODIR=.obj
LDIR = lib

#LIBS=-l

_DEPS = xhyvectl.h
DEPS = $(patsubst %,$(IDIR)/%,$(_DEPS))

_OBJ = xhyvectl.o ini.o
OBJ = $(patsubst %,$(ODIR)/%,$(_OBJ))

$(ODIR)/%.o: src/%.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

xhyvectl: $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

.PHONY: clean

clean:
	rm -f $(ODIR)/*.o *~ core $(INCDIR)/*~ 
