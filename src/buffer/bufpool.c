#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>

#include "buffer/bufpool.h"
#include "global/config.h"
#include "storage/page.h"

extern Config* conf;

BufPool* bufpool_init(int size) {
  BufPool* bp = malloc(sizeof(BufPool));
  bp->size = size;
  bp->pages = calloc(size, sizeof(Page));

  // initialize each slot with a blank page
  for (int i = 0; i < size; i++) {
    bp->pages[i] = new_page();
  }

  return bp;
}

void bufpool_destroy(BufPool* bp) {
  if (bp == NULL) return;

  if (bp->pages != NULL) {
    for (int i = 0; i < bp->size; i++) {
      if (bp->pages[i] != NULL) free_page(bp->pages[i]);
    }

    free(bp->pages);
  }

  free(bp);
}

/**
 * @brief Loads the requested page described by `tag` into slot `bufId` of
 * the buffer pool
 * 
 * @param fd 
 * @param bp 
 * @param bufId 
 * @param tag 
 * @return true 
 * @return false 
 */
bool bufpool_read_page(FileDescList* fdl, BufPool* bp, int32_t bufId, BufTag* tag) {
  FileDesc* fdesc = buffile_open(fdl, tag->fileId);

  if (fdesc == NULL) return false;

  lseek(fdesc->fd, (tag->pageId - 1) * conf->pageSize, SEEK_SET);
  int bytes_read = read(fdesc->fd, bp->pages[bufId], conf->pageSize);

  if (bytes_read != conf->pageSize) {
    printf("Bytes read: %d\n", bytes_read);
    return false;
  }

  return true;
}

bool bufpool_flush_page(FileDescList* fdl, BufDescArr* bd, BufPool* bp, int32_t bufId) {
  BufDesc* bdesc = (BufDesc*)bd->descArr[bufId];
  FileDesc* fdesc = buffile_open(fdl, bdesc->tag->fileId);

  lseek(fdesc->fd, (bdesc->tag->pageId - 1) * conf->pageSize, SEEK_SET);
  int bytes_written = write(fdesc->fd, bp->pages[bufId], conf->pageSize);

  if (bytes_written != conf->pageSize) return false;

  return true;
}