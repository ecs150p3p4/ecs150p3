# Target library
lib := libuthread.a

all: $(lib)

## TODO: Phase 1 and Phase 2

objs := sems.o tps.o 

CC := gcc
CFLAGS := -Wall -Werror


libuthread.a: $(objs)
	ar rcs $@ $^

sem.o: sem.c sem.h
	$(CC) $(CFLAGS) -c -o $@ $<

tps.o: tps.c tps.h
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f $(lib) $(objs)
