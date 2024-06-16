#include <stdlib.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>

#include "buffer/buffile.h"
#include "utility/linkedlist.h"
#include "global/config.h"

extern Config* conf;

FileDescList* buffile_init() {
  return new_linkedlist();
}

static void buffile_cleanup(void* fdesc) {
  FileDesc* f = (FileDesc*)fdesc;
  close(f->fd);
  free(f->filename);
  free(f);
}

void buffile_destroy(FileDescList* fdl) {
  free_linkedlist(fdl, &buffile_cleanup);
}

static bool buffile_comparison(void* f1, void* f2) {
  FileDesc* fdesc = (FileDesc*)f1;
  uint32_t fileId = *(uint32_t*)f2;

  if (fdesc->fileId == fileId) return true;
  return false;
}

FileDesc* buffile_search(FileDescList* fdl, uint32_t fileId) {
  ListItem* li = linkedlist_search(fdl, &fileId, buffile_comparison);

  if (li == NULL) return NULL;
  return (FileDesc*)li->ptr;
}

FileDesc* buffile_open(FileDescList* fdl, uint32_t fileId) {
  FileDesc* fdesc = buffile_search(fdl, fileId);

  if (fdesc != NULL) return fdesc;

  fdesc = malloc(sizeof(FileDesc));
  fdesc->fileId = fileId;

  if (fileId == FILE_DATA) {
    fdesc->fd = open(conf->dataFile,
              O_RDWR |    // Read/Write mode
                O_CREAT,  // Create file if it doesn't exist
              S_IWUSR |   // User write permission
                S_IRUSR   // User read permission
    );
    fdesc->filename = malloc(strlen(conf->dataFile) + 1);
    strcpy(fdesc->filename, conf->dataFile);
  } else if (fileId == FILE_LOG) {
    // write code for the log file
    printf("Unhandled log file\n");
    free(fdesc);
    return NULL;
  } else {
    printf("Unknown fileId: %d\n", fileId);
    free(fdesc);
    return NULL;
  }

  int length = lseek(fdesc->fd, 0, SEEK_END);
  fdesc->nextPageId = (length / conf->pageSize) + 1;

  linkedlist_append(fdl, fdesc);

  return fdesc;
}

/**
 * @brief Claims the nextPageId for the calling process. The caller is
 * responsible for writing the page to disk then releasing the lock on
 * the file
 * 
 * @param fdl 
 * @param fileId 
 * @return uint32_t 
 */
uint32_t buffile_get_new_pageid(FileDescList* fdl, uint32_t fileId) {
  /**
   * @todo This is where we take a brief lock on the file to ensure
   * the caller is the only one that can claim the nextPageId
   */
  FileDesc* fdesc = buffile_open(fdl, fileId);
  uint32_t newPageId = fdesc->nextPageId;
  fdesc->nextPageId++;
  return newPageId;
}

void buffile_diag_summary(FileDescList* fdl) {
  printf("----------------------------------\n");
  printf("---     Buffer File Summary    ---\n");
  printf("----------------------------------\n");
  
  ListItem* li = fdl->head;

  while (li != NULL) {
    FileDesc* fdesc = (FileDesc*)li->ptr;
    printf("= Filename: %s\n", fdesc->filename);
    printf("= FileId:   %d\n", fdesc->fileId);
    printf("----------------------------------\n");

    li = li->next;
  }
}