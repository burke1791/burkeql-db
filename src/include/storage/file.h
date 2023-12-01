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