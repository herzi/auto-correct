all: auto-correct

auto-correct: main.c Makefile
	gcc -O2 -Wall -o $@ $< $(shell pkg-config --cflags --libs gtk+-2.0 libxml-2.0)

check:
	xmllint --noout --schema XMLSchema.xsd XMLSchema.xsd
	xmllint --noout --schema XMLSchema.xsd auto-correct.xsd
	xmllint --noout --schema auto-correct.xsd auto-correct.xml

clean:
	rm -rf auto-correct
