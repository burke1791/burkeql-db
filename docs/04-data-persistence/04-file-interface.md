# File Interface

The file interface is going to be really simple. All we need to do is create a struct that holds a file descriptor and two functions to open and close files. Let's start by writing the header file:

`src/include/storage/file.h`

```c
typedef struct FileDesc {
  char* filename;
  int fd;
} FileDesc;

FileDesc* file_open(char* filename);
void file_close(FileDesc* fdesc);
```

The `FileDesc` struct (short for FileDescriptor) keeps track of the file's name and an int representing the file descriptor. The `file_open` function is responsible for allocating memory for the `FileDesc` object and opening the provided file. And the `file_close` function simply closes the open file and frees the memory used by `FileDesc`.

## Opening and Closing Files

We'll be using the `open` and `close` system calls to interact with any files our program needs. To open a file, we first allocate memory for our file descriptor struct, then attempt to open the file. If `open` returns `-1`, then we free the file descriptor and return a `NULL`. If `open` succeeds, we set the `filename` property and return the file descriptor object.

```c
FileDesc* file_open(char* filename) {
  FileDesc* fdesc = malloc(sizeof(FileDesc));

  fdesc->fd = open(filename,
                O_RDWR |    // read/write mode
                  O_CREAT,  // create file if it doesn't exist
                S_IWUSR |   // user write permission
                  S_IRUSR   // user read permission
  );

  if (fdesc->fd == -1) {
    free(fdesc);
    return NULL;
  }

  fdesc->filename = strdup(filename);

  return fdesc;
}
```

Closing a file is just as straightforward. The only extra thing we need to do is free the memory allocated for the file descriptor and its elements.

```c
void file_close(FileDesc* fdesc) {
  if (fdesc->fd != -1) {
    close(fdesc->fd);
  }

  if (fdesc->filename != NULL) free(fdesc->filename);

  free(fdesc);
}
```

That's it. We'll be using these two functions to read in the config settings, as well as reading from the actual data file.

## Full Files

`src/include/storage/file.h`

```c
#ifndef FILE_H
#define FILE_H

#include <stdint.h>
#include <stdio.h>

typedef struct FileDesc {
  char* filename;
  int fd;
} FileDesc;

FileDesc* file_open(char* filename);
void file_close(FileDesc* fdesc);

#endif /* FILE_H */
```

`src/storage/file.c`

```c
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <stdio.h>
#include <unistd.h>

#include "storage/file.h"

FileDesc* file_open(char* filename) {
  FileDesc* fdesc = malloc(sizeof(FileDesc));

  fdesc->fd = open(filename,
                O_RDWR |    // read/write mode
                  O_CREAT,  // create file if it doesn't exist
                S_IWUSR |   // user write permission
                  S_IRUSR   // user read permission
  );

  if (fdesc->fd == -1) {
    free(fdesc);
    return NULL;
  }

  fdesc->filename = strdup(filename);

  return fdesc;
}

void file_close(FileDesc* fdesc) {
  if (fdesc->fd != -1) {
    close(fdesc->fd);
  }

  if (fdesc->filename != NULL) free(fdesc->filename);

  free(fdesc);
}
```