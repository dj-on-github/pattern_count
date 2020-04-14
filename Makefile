CC = gcc
CFLAGS = -I/usr/local/include -m64 -Wall -O3
LDFLAGS = -L/usr/local/lib 
LDLIBS = 

pattern_count: pattern_count.o 
	$(CC) $(CFLAGS) $(LDFLAGS) pattern_count.o -o pattern_count $(LDLIBS)

pattern_count.o: pattern_count.c
	$(CC) -c $(CFLAGS) -o pattern_count.o pattern_count.c


install:
	cp pattern_count /usr/local/bin

clean:
	rm pattern_count.o
	rm pattern_count

