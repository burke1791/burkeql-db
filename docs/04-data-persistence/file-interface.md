# File Interface

The file interface is going to be really simple. All we need to do is create a struct that holds a pointer to a `FILE` type and two functions to open and close files. Let's start by writing the header file:

`src/include/storage/file.h`

```c
typedef struct FileDesc {
  char* filename;
  FILE* fp;
} FileDesc;

FileDesc* file_open(char* filename, char* mode);
void file_close(FileDesc* fd);
```

The `FileDesc` struct (short for FileDescriptor) keeps track of the file's name and a pointer to the open file. The `file_open` function is responsible for allocating memory for the `FileDesc` object and opening the provided file. And the `file_close` function simply closes the open file and frees the memory used by `FileDesc`.

## Opening and Closing Files

We'll be using the `fopen` and `fclose` standard functions to interact with any files our program needs. To open a file, we first allocate memory for our file descriptor struct, then attempt to open the file. If `fopen` returns `NULL`, then we free the file descriptor and return a `NULL`. If `fopen` succeeds, we set the `filename` property and return the file descriptor object.

```c
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
```

Closing a file is just as straightforward. The only extra thing we need to do is free the memory allocated for the file descriptor and its elements.

```c
void file_close(FileDesc* fd) {
  if (fd->fp != NULL) {
    fclose(fd->fp);
  }

  if (fd->filename != NULL) free(fd->filename);

  free(fd);
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
  FILE* fp;
} FileDesc;

FileDesc* file_open(char* filename, char* mode);
void file_close(FileDesc* fd);

#endif /* FILE_H */
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