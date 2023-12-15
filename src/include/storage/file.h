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