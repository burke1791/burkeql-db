# Loading Config

Now that we have functions to open and close files for us, we need to write the code that reads the config file line-by-line and sets the valid values in a global `Config` object. Let's begin by writing the header file for our config code:

```c
typedef enum ConfigParameter {
  CONF_DATA_FILE,
  CONF_UNRECOGNIZED
} ConfigParameter;

typedef struct Config {
  char* dataFile;
} Config;
```

The `ConfigParameter` enum is going to be the list of valid config options to put in our `burkeql.conf` file. This list will grow as we add more to the database system, but for now all we need is the data file location and a value for an unrecognized parameter.

The `Config` struct is going to be a global object initialized at the beginning of the `main` function, and populated shortly thereafter. Except for `CONF_UNRECOGNIZED`, the properties in the `Config` struct will correspond one-to-one with the enum values.

Next, we need to define functions we want to expose to the rest of our program.

```c
Config* new_config();
void free_config(Config* conf);

bool set_global_config(Config* conf);

void print_config(Config* conf);
```

The first two are just responsible for allocating and freeing memory associated with the `Config` object.

`set_global_config` is the workhorse. It will be called towards the beginning of `main()` and is responsible for opening the config file, reading its contents, and setting the provided values in the `Config` object. Returns `true` if it succeeds, and `false` otherwise.

`print_config` is just there for debugging. After we set the config values, we'll want to print them to the terminal just to make sure everything went as expected.

## Implementation

Let's start with the simple functions, `new_config`, `free_config`, and `print_config`:

```c
Config* new_config() {
  Config* conf = malloc(sizeof(Config));
  return conf;
}

void free_config(Config* conf) {
  if (conf->dataFile != NULL) free(conf->dataFile);
  free(conf);
}

void print_config(Config* conf) {
  printf("======   BurkeQL Config   ======\n");
  printf("= DATA_FILE: %s\n", conf->dataFile);
}
```

Very straightforward. Allocate the memory we need. Free everything that was allocated. And print the only config property we currently have.

The fourth function has a lot more going on by comparison, and even utilizes some `static` helper functions:

```c
bool set_global_config(Config* conf) {
  FileDesc* fd = file_open("burkeql.conf", "r");
  char* line = NULL;
  size_t len = 0;
  ssize_t read;
  ConfigParameter p;

  if (fd->fp == NULL) {
    printf("Unable to read config file: burkeql.conf\n");
    return false;
  }
```

We start by declaring/initializing the variables we'll need when we loop through the config file. Then we have a simple error check that bails out early if there's an issue.

```c
  /**
   * loops through the config file and sets values in the
   * global Config object
   */
  while ((read = getline(&line, &len, fd->fp)) != -1) {
    // skip comment lines or empty lines
    if (strncmp(line, "#", 1) == 0 || read <= 1) continue;

    // parse out the key and value
    char* param = strtok(line, "=");
    char* value = strtok(NULL, "=");

    p = parse_config_param(param);

    if (p == CONF_UNRECOGNIZED) continue;

    set_config_value(conf, p, value);
  }
```

This is the meat of our `set_global_config` function. We use a fairly common loop condition to read through a file one line at a time until it hits an `EOF`. We then check if the line begins with a `#` character, indicating a comment line, or if the length of the line is 0, indicating an empty line. If either of those conditions is met, then we skip to the next line.

Next, we use the `strtok` function to split the parameter name and its value into separate variables. Since we know they are separated by an "=" symbol, we pass that as the second argument to the function. Then we use one of our helper functions to match the value of `param` to one of the enum values in `ConfigParameter`. If it's a valid parameter, we call our second helper function to set the value in our global object.

Finally, we free up any memory we allocated for this function:

```c
  if (line) free(line);

  file_close(fd);

  return true;
}
```

`parse_config_param` is a really simple function. Just a bunch of `if(strcmp(...` statements to determine the correct enum value:

```c
static ConfigParameter parse_config_param(char* p) {
  if (strcmp(p, "DATA_FILE") == 0) return CONF_DATA_FILE;

  return CONF_UNRECOGNIZED;
}
```

`set_config_value` follows a similar pattern, except we run a `switch` statement on the enum value returned by the above function:

```c
static void set_config_value(Config* conf, ConfigParameter p, char* v) {
  switch (p) {
    case CONF_DATA_FILE:
      v[strcspn(v, "\r\n")] = 0; // remove trailing newline character if it exists
      conf->dataFile = strdup(v);
      break;
  }
}
```

## Updating `main()`

The change to `main.c` is pretty simple. We just need to include the new `config.h` header and write the code to load config values into the global `Config` object.

```diff
 #include "parser/parsetree.h"
 #include "parser/parse.h"
+#include "global/config.h"
+
+Config* conf;

 static void print_prompt() {
   printf("bql > ");
 }
 
 int main(int argc, char** argv) {
+  // initialize global config
+  conf = new_config();
+
+  if (!set_global_config(conf)) {
+    return EXIT_FAILURE;
+  }
+
+  // print config
+  print_config(conf);
 
   while(true) {
     print_prompt();
```

## Makefile

Lastly, we need to update the `Makefile` by adding the new c files to the `SRC_FILES` variable.

```diff
SRC_FILES = main.c \
 						parser/parse.c \
+						parser/parsetree.c \
+						global/config.c \
+						storage/file.c
```

## Running the Program

Before you run the program, make sure you create a `burkeql.conf` file in whatever directory you'll execute the program from. I do everything at the root level of the code repository to keep it simple. My config file looks like this:

`burkeql.conf`

```conf
# Config file for the BurkeQL database

# Absolute location of the data file
DATA_FILE=/home/burke/source_control/burkeql-db/db_files/main.dbd
```

Now we can compile and run the program to see if our config file is properly parsed. Remember, you can safely ignore warnings about functions produced by flex and bison (e.g. `yylex`, `yyerror`, etc.).

```shell
$ ./burkeql
======   BurkeQL Config   ======
= DATA_FILE: /home/burke/source_control/burkeql-db/db_files/main.dbd
bql > \quit
======  Node  ======
=  Type: SysCmd
=  Cmd: quit
Shutting down...
$ 
```

As you can see, our code correctly reads and parses the config file and stores the value in the global config object. Keep in mind the parser we wrote for the config file is extremely basic. That means it's easy to break it. However, we do kill the program if there's an issue parsing the config file, so it shouldn't be too dangerous to leave it as is.

## Full Files

Here's the folder layout I have at this point:

```shell
├── Makefile
├── burkeql.conf
├── db_files       <-- Not necessary yet, but will be in the next section
└── src
    ├── Makefile
    ├── global
    │   └── config.c
    ├── include
    │   ├── global
    │   │   └── config.h
    │   ├── parser
    │   │   ├── parse.h
    │   │   └── parsetree.h
    │   └── storage
    │       └── file.h
    ├── main.c
    ├── parser
    │   ├── gram.y
    │   ├── parse.c
    │   ├── parsetree.c
    │   └── scan.l
    └── storage
        └── file.c
```

And the current state of all the files we changed.

`src/Makefile`

```shell
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
						storage/file.c


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
```

`src/include/global/config.h`

```c
#ifndef CONFIG_H
#define CONFIG_H

#include <stdbool.h>

typedef enum ConfigParameter {
  CONF_DATA_FILE,
  CONF_UNRECOGNIZED
} ConfigParameter;

typedef struct Config {
  char* dataFile;
} Config;

Config* new_config();
void free_config(Config* conf);

bool set_global_config(Config* conf);

void print_config(Config* conf);

#endif /* CONFIG_H */
```

`src/include/storage/file.h`

```c
#ifndef FILE_H
#define FILE_H

#include <stdint.h>
#include <stdio.h>

typedef struct FileDesc {
  char* filename;
  FILE* fp;
} FileDesc;

FileDesc* file_open(char* filename, char* mode);
void file_close(FileDesc* fd);

#endif /* FILE_H */
```

`src/global/config.c`

```c
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "global/config.h"
#include "storage/file.h"

Config* new_config() {
  Config* conf = malloc(sizeof(Config));
  return conf;
}

void free_config(Config* conf) {
  if (conf->dataFile != NULL) free(conf->dataFile);
  free(conf);
}

void print_config(Config* conf) {
  printf("======   BurkeQL Config   ======\n");
  printf("= DATA_FILE: %s\n", conf->dataFile);
}

static ConfigParameter parse_config_param(char* p) {
  if (strcmp(p, "DATA_FILE") == 0) return CONF_DATA_FILE;

  return CONF_UNRECOGNIZED;
}

static void set_config_value(Config* conf, ConfigParameter p, char* v) {
  switch (p) {
    case CONF_DATA_FILE:
      v[strcspn(v, "\r\n")] = 0; // remove trailing newline character if it exists
      conf->dataFile = strdup(v);
      break;
  }
}

bool set_global_config(Config* conf) {
  FileDesc* fd = file_open("burkeql.conf", "r");
  char* line = NULL;
  size_t len = 0;
  ssize_t read;
  ConfigParameter p;

  if (fd->fp == NULL) {
    printf("Unable to read config file: burkeql.conf\n");
    return false;
  }

  /**
   * loops through the config file and sets values in the
   * global Config object
   */
  while ((read = getline(&line, &len, fd->fp)) != -1) {
    // skip comment lines or empty lines
    if (strncmp(line, "#", 1) == 0 || read <= 1) continue;

    // parse out the key and value
    char* param = strtok(line, "=");
    char* value = strtok(NULL, "=");

    p = parse_config_param(param);

    if (p == CONF_UNRECOGNIZED) continue;

    set_config_value(conf, p, value);
  }

  if (line) free(line);

  file_close(fd);

  return true;
}
```

`src/storage/file.c`

```c
#include <string.h>
#include <stdlib.h>

#include "storage/file.h"

FileDesc* file_open(char* filename, char* mode) {
  FileDesc* fd = malloc(sizeof(FileDesc));

  fd->fp = fopen(filename, mode);

  if (fd->fp == NULL) {
    free(fd);
    return NULL;
  }

  fd->filename = strdup(filename);

  return fd;
}

void file_close(FileDesc* fd) {
  if (fd->fp != NULL) {
    fclose(fd->fp);
  }

  if (fd->filename != NULL) free(fd->filename);

  free(fd);
}
```

`src/main.c`

```c
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#include "gram.tab.h"
#include "parser/parsetree.h"
#include "parser/parse.h"
#include "global/config.h"

Config* conf;

static void print_prompt() {
  printf("bql > ");
}

int main(int argc, char** argv) {
  // initialize global config
  conf = new_config();

  if (!set_global_config(conf)) {
    return EXIT_FAILURE;
  }

  // print config
  print_config(conf);

  while(true) {
    print_prompt();
    Node* n = parse_sql();

    if (n == NULL) continue;

    switch (n->type) {
      case T_SysCmd:
        if (strcmp(((SysCmd*)n)->cmd, "quit") == 0) {
          print_node(n);
          free_node(n);
          printf("Shutting down...\n");
          return EXIT_SUCCESS;
        }
      default:
        print_node(n);
        free_node(n);
    }
  }

  return EXIT_SUCCESS;
}
```