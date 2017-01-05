# Makefile of banker's algorithm
# Jacob @ 2016

BANKER := banker
CFLAGS += -g
CFLAGS += -Wall -lpthread

all: $(BANKER)

$(BANKER): $(BANKER).o
	$(CC) -o $@ $< $(CFLAGS)

.c.o:
	$(CC) -o $@ -c $?

clean:
	rm -f banker.o banker
