CC = gcc
LEX = flex
YACC = bison
CFLAGS = -I./ -I./include -fsanitize=address -fsanitize=undefined -static-libasan -g

TARGET_EXEC = burkeql

BUILD_DIR = ..

SRC_FILES = main.c \
						parser/parse.c \
						parser/parsetree.c \
						global/config.c \
						storage/file.c \
						storage/page.c \
						storage/record.c \
						storage/datum.c


$(BUILD_DIR)/$(TARGET_EXEC): gram.tab.o lex.yy.o ${SRC_FILES}
	${CC} ${CFLAGS} -o $@ $?

gram.tab.c gram.tab.h: parser/gram.y
	${YACC} -vd $?

lex.yy.c: parser/scan.l
	${LEX} -o $*.c $<

lex.yy.o: gram.tab.h lex.yy.c

clean:
	rm -f $(wildcard *.o)
	rm -f $(wildcard *.output)
	rm -f $(wildcard *.tab.*)
	rm -f lex.yy.c
	rm -f $(wildcard *.lex.*)
	rm -f $(BUILD_DIR)/$(TARGET_EXEC)