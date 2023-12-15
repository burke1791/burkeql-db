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



/**
 * @brief closes the FILE* referenced in the FileDesc struct and frees FileDesc
 * 
 * @param fd 
 */
void file_close(FileDesc* fdesc) {
  if (fdesc->fd != -1) {
    close(fdesc->fd);
  }

  if (fdesc->filename != NULL) free(fdesc->filename);

  free(fdesc);
}