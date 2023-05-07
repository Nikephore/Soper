CC=gcc
CFLAGS=-Wall -Wextra -pedantic 
LDFLAGS=-lrt -lm
LIBS = -lpthread

all: clean monitor miner

monitor: monitor.o pow.o common.o
	$(CC) $(LDFLAGS) $^ -o $@ 

miner: minero.o pow.o common.o 
	$(CC) $(LDFLAGS) $^ -o $@ $(LIBFLAGS)

monitor.o: monitor.c pow.h common.h
	$(CC) $(CFLAGS) -c $< -o $@

minero.o: minero.c pow.h common.h
	$(CC) $(CFLAGS) -c $< -o $@

common.o: common.c common.h
	$(CC) $(CFLAGS) -c $< -o $@

pow.o: pow.c pow.h
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f monitor miner pow *.o