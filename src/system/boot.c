#include <string.h>
#include <stdlib.h>

#include "system/boot.h"
#include "global/config.h"

extern Config* conf;

bool init_boot_page(BufMgr* buf, BufTag* tag) {
  tag->fileId = FILE_DATA;
  tag->pageId = BOOT_PAGE_ID;
  
  int32_t bufId = bufmgr_request_bufId(buf, tag);
  if (bufId < 0) {
    bufId = bufmgr_allocate_new_page(buf, FILE_DATA);
    if (buf->bd->descArr[bufId]->tag->pageId != BOOT_PAGE_ID) {
      printf("Allocated page is not the boot page\n");
      return false;
    }
  }
  page_zero(buf->bp->pages[bufId]);

  set_major_version(buf, bufId, MAJOR_VERSION);
  set_minor_version(buf, bufId, MINOR_VERSION);
  set_patch_num(buf, bufId, PATCH_NUM);
  set_page_size(buf, bufId, conf->pageSize);

  flush_boot_page(buf);

  return true;
}

static int32_t get_boot_page_bufid(BufMgr* buf) {
  BufTag* tag = bufdesc_new_buftag(FILE_DATA, BOOT_PAGE_ID);
  int32_t bufId = bufmgr_request_bufId(buf, tag);
  bufdesc_free_buftag(tag);

  return bufId;
}

void set_major_version(BufMgr* buf, int32_t bufId, uint16_t val) {
  memcpy(buf->bp->pages[bufId] + MAJOR_VERSION_BYTE_POS, &val, MAJOR_VERSION_BYTE_SIZE);
}

void set_minor_version(BufMgr* buf, int32_t bufId, uint32_t val) {
  memcpy(buf->bp->pages[bufId] + MINOR_VERSION_BYTE_POS, &val, MINOR_VERSION_BYTE_SIZE);
}

void set_patch_num(BufMgr* buf, int32_t bufId, uint32_t val) {
  memcpy(buf->bp->pages[bufId] + PATCH_NUM_BYTE_POS, &val, PATCH_NUM_BYTE_SIZE);
}

void set_page_size(BufMgr* buf, int32_t bufId, uint16_t val) {
  memcpy(buf->bp->pages[bufId] + PAGE_SIZE_BYTE_POS, &val, PAGE_SIZE_BYTE_SIZE);
}

uint16_t get_major_version(BufMgr* buf) {
  int32_t bufId = get_boot_page_bufid(buf);

  uint16_t majorVersion;

  memcpy(&majorVersion, buf->bp->pages[bufId] + MAJOR_VERSION_BYTE_POS, MAJOR_VERSION_BYTE_SIZE);

  return majorVersion;
}

uint32_t get_minor_version(BufMgr* buf) {
  int32_t bufId = get_boot_page_bufid(buf);

  uint32_t minorVersion;

  memcpy(&minorVersion, buf->bp->pages[bufId] + MINOR_VERSION_BYTE_POS, MINOR_VERSION_BYTE_SIZE);

  return minorVersion;
}

uint32_t get_patch_num(BufMgr* buf) {
  int32_t bufId = get_boot_page_bufid(buf);

  uint32_t patchNum;

  memcpy(&patchNum, buf->bp->pages[bufId] + PATCH_NUM_BYTE_POS, PATCH_NUM_BYTE_SIZE);

  return patchNum;
}

uint16_t get_page_size(BufMgr* buf) {
  int32_t bufId = get_boot_page_bufid(buf);

  uint16_t pageSize;

  memcpy(&pageSize, buf->bp->pages[bufId] + PAGE_SIZE_BYTE_POS, PAGE_SIZE_BYTE_SIZE);

  return pageSize;
}


void flush_boot_page(BufMgr* buf) {
  BufTag* tag = malloc(sizeof(BufTag));
  tag->fileId = FILE_DATA;
  tag->pageId = BOOT_PAGE_ID;
  bufmgr_flush_page(buf, tag);
  free(tag);
}