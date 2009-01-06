all: auto-correct

auto_correct_SOURCES=\
	ac-auto-correction.c \
	ac-auto-correction.h \
	main.c \
	$(NULL)

auto-correct: $(auto_correct_SOURCES) Makefile
	gcc -O2 -Wall -o $@ $(auto_correct_SOURCES) $(shell pkg-config --cflags --libs gtk+-2.0 libxml-2.0) \
		|| rm $@

check:
	xmllint --nonet --noout --schema XMLSchema.xsd XMLSchema.xsd
	xmllint --nonet --noout --schema XMLSchema.xsd auto-correct.xsd
	xmllint --nonet --noout --schema auto-correct.xsd auto-correct.xml

clean:
	rm -rf auto-correct
