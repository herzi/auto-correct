all: auto-correct

auto_correct_SOURCES=\
	ac-auto-correction.c \
	ac-auto-correction.h \
	main.c \
	$(NULL)
test_ac_SOURCES=\
	ac-auto-correction.c \
	ac-auto-correction.h \
	test-ac.c \
	$(NULL)

auto-correct: $(auto_correct_SOURCES) Makefile
	gcc -O2 -Wall -o $@ $(auto_correct_SOURCES) $(shell pkg-config --cflags --libs gtk+-2.0 libxml-2.0) \
		|| rm $@

test-ac: $(test_ac_SOURCES) Makefile
	gcc -O2 -Wall -o $@ $(test_ac_SOURCES) $(shell pkg-config --cflags --libs gtk+-2.0 libxml-2.0) \
		|| rm $@

check: test-ac Makefile
	xmllint --nonet --noout --schema XMLSchema.xsd XMLSchema.xsd
	xmllint --nonet --noout --schema XMLSchema.xsd auto-correct.xsd
	xmllint --nonet --noout --schema auto-correct.xsd auto-correct.xml
	gtester --verbose $<

clean:
	rm -rf auto-correct
