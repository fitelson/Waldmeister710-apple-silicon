all: wm-lex.c wm-parse.tab.c

CC=gcc
CFLAGS=-W -g

wm-lex.c: wm-lex.l wm-parse.tab.h
	flex -owm-lex.c wm-lex.l
#	flex -pp -v -owm-lex.c wm-lex.l

wm-parse.tab.c wm-parse.tab.h: wm-parse.y Makefile parse-util.h
	bison -d -v wm-parse.y

parse-util.c: parse-util.h

tester: wm-lex.o wm-parse.tab.o tester.o parse-util.o

.PHONY: clean
clean:
	rm -f tester tester.o wm-lex.o wm-parse.output wm-parse.tab* parse-util.o core