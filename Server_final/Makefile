CC=gcc
CFLAGS=-c
LDFLAGS=-pthread -lm -lfann -lmraa
SOURCES=main_v8.c server.c queue.c
OBJECTS=$(SOURCES:.c=.o)
EXECUTABLE=Main

all: $(SOURCES) $(EXECUTABLE)
    
$(EXECUTABLE): $(OBJECTS) 
	$(CC) $(LDFLAGS) $(OBJECTS) -o $@

.c.o:
	$(CC) $(CFLAGS) $< -o $@

clean:
	rm -rf $(EXECUTABLE)
	rm -rf $(OBJECTS)

clobber: clean
	rm -rf *~
