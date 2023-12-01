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
}

/**
 * @brief closes the FILE* referenced in the FileDesc struct and frees FileDesc
 * 
 * @param fd 
 */
void file_close(FileDesc* fd) {
  if (fd->fp != NULL) {
    fclose(fd->fp);
  }

  if (fd->filename != NULL) free(fd->filename);

  free(fd);
}