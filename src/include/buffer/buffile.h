#ifndef BUFFILE_H
#define BUFFILE_H

#include <stdint.h>
#include <stdbool.h>

#include "utility/linkedlist.h"

#define FILE_DATA 1
#define FILE_LOG 2

typedef struct FileDesc {
  char* filename;
  uint32_t fileId;
  uint32_t nextPageId;
  int fd;
} FileDesc;

typedef struct LinkedList FileDescList;

FileDescList* buffile_init();
void buffile_destroy(FileDescList* fdl);

FileDesc* buffile_open(FileDescList* fdl, uint32_t fileId);
// void buffile_close(FileDescList* fdl, uint32_t fileId);
FileDesc* buffile_search(FileDescList* fdl, uint32_t fileId);

uint32_t buffile_get_new_pageid(FileDescList* fdl, uint32_t fileId);

void buffile_diag_summary(FileDescList* fdl);

#endif /* BUFFILE_H */