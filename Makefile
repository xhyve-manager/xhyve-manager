include config.mk

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

$(TARGET): $(TARGET).o ini.o
	$(CC) -o $(TARGET) $(TARGET).o ini.o -I.

clean:
	rm $(TARGET) && rm *.o

