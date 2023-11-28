#ifndef FILE_H
#define FILE_H

#include <stdint.h>

typedef struct FileDesc {
  char* filename;
  int fd;
} FileDesc;

FileDesc* file_open(char* filename);
void file_close(int fd);

#endif /* FILE_H */