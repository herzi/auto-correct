all: auto-correct

auto-correct: main.c Makefile
	gcc -O2 -Wall -o $@ $< $(shell pkg-config --cflags --libs gtk+-2.0 libxml-2.0)

clean:
	rm -rf auto-correct
