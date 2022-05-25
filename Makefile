.PHONY: all run clean

CC = gcc
CFLAGS = -ggdb -std=c99 -Wall -Wextra -Werror
CFLAGS += -Wno-unused-label -Wno-unused-parameter -Wno-unused-function -Wno-unused-variable

all: ./build/crdp

./build/%.o: %.c
	mkdir -p $(@D)
	$(CC) $(CFLAGS) -c -o $@ $<

./build/crdp: ./build/main.o
	mkdir -p $(@D)
	$(CC) -o $@ $^

run: ./build/crdp
	./build/crdp

clean:
	rm -rf build/
