CC = gcc
CFLAGS = -Wall -pedantic
LIBS = -lpthread
EXE = voting

.PHONY: all clear clean

clean:
	rm -f *.o

$(EXE): voting.o voter.o pow.o
	$(CC) $(CFLAGS) -o $@ $^ $(LIBFLAGS)

all: clean $(EXE)

voting.o: voting.c voting.h
	$(CC) $(CFLAGS) -c $<

voter.o: voter.c voter.h
	$(CC) $(CFLAGS) -c $<

pow.o: pow.c pow.h
	$(CC) $(CFLAGS) -c $<

