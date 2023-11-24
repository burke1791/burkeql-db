# The `main.c`

The last piece we need is an outer program that can call the parser. We define our `main.c` as follows:

`src/main.c`

```c
#include <stdio.h>
#include <stdlib.h>

#include "gram.tab.h"

static void print_prompt() {
  printf("bql > ");
}

int main(int argc, char** argv) {
  print_prompt();

  while (!yyparse()) {
    print_prompt();
  }

  return EXIT_SUCCESS;
}
```

Our program prints out a prompt and waits for user input. Upon a successful parse, it prints the prompt again and waits for more input.

Note: before you compile everything, `gram.tab.h` won't exist, so your intellisense will probably yell at you. This is fine and you can ignore it.

## Makefiles

Just like I didn't dive into flex and bison, I'm also not going to dive into makefiles. Mostly because I'm still a beginner at them myself. But, here are the makefiles that will build a working program:

### Inner Makefile

`src/Makefile`

```bash
CC = gcc
LEX = flex
YACC = bison
CFLAGS = -fsanitize=address -static-libasan -g

TARGET_EXEC = burkeql

BUILD_DIR = ..

SRC_FILES = main.c


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
	rm -f $(BUILD_DIR)/$(TARGET_EXEC)
```

### Outer Makefile

The entire purpose of this inner Makefile is because I am too lazy to type `cd src && make`. I just want to be able to call all make functionality from the same directory I can call my git commands from.

`Makefile`

```c
all:
	cd src && $(MAKE)

clean:
	rm -f ./bql
	cd src && $(MAKE) clean
```

## Current Directory Tree

At this point, our project should look like this:

```bash
├── Makefile
└── src
    ├── Makefile
    ├── main.c
    └── parser
        ├── gram.y
        └── scan.l
```

## Running the Program

```shell
$ make
*** There will be several warnings, but you can safely ignore them ***
make[1]: Leaving directory '***'
$ ./burkeql
bql > select
SELECT command received
bql >       insert
INSERT command received
bql > quit
QUIT command received
bql > foo
error: unknown character 'f'
error: unknown character 'o'
error: unknown character 'o'

```

As you can see the parser is able to recognize our three commands, and even gracefully ignore whitespace. However, the error handling is quite pedestrian. Don't worry, we'll fix it later.