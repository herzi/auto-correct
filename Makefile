all: auto-correct

auto-correct: main.c Makefile
	gcc -O2 -Wall -o $@ $<

clean:
	rm -rf auto-correct
