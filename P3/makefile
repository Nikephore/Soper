CC=gcc
CFLAGS=-Wall -Wextra -pedantic -std=c99
LDFLAGS=-lrt -lm

monitor: monitor.o pow.o
	$(CC) $(LDFLAGS) $^ -o $@

minero: minero.o pow.o
	$(CC) $(LDFLAGS) $^ -o $@

monitor.o: monitor.c pow.h
	$(CC) $(CFLAGS) -c $< -o $@

minero.o: minero.c pow.h
	$(CC) $(CFLAGS) -c $< -o $@

pow.o: pow.c pow.h
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f monitor minero *.o