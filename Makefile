include config.mk

_DEPS = xhyvectl.h
DEPS = $(patsubst %,$(IDIR)/%,$(_DEPS))

_OBJ = xhyvectl.o ini.o
OBJ = $(patsubst %,$(ODIR)/%,$(_OBJ))

$(ODIR):
	@mkdir -p $(ODIR)

$(ODIR)/%.o: src/%.c $(DEPS) | $(ODIR)
	$(CC) -c -o $@ $< $(CFLAGS)

xhyvectl: $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

.PHONY: clean

clean:
	rm -f $(ODIR)/*.o *~ core $(INCDIR)/*~ 
