CC = gcc
CFLAGS = -Wall -pedantic
LIBS = -lpthread
EXE = mrush

.PHONY: all clear clean

clean:
	rm -f *.o

$(EXE): mrush.c minero.o pow.o monitor.o
	$(CC) $(CFLAGS) -o $@ $^ $(LIBFLAGS)

all: clean $(EXE)

minero.o: minero.c minero.h
	$(CC) $(CFLAGS) -c $<

monitor.o: monitor.c monitor.h
	$(CC) $(CFLAGS) -c $<

pow.o: pow.c pow.h
	$(CC) $(CFLAGS) -c $<