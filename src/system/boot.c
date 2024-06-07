#include <string.h>

#include "system/boot.h"
#include "global/config.h"

extern Config* conf;

static int32_t get_boot_page_bufid(BufMgr* buf) {
  int32_t bufId = bufmgr_request_bufId(buf, BOOT_PAGE_ID);

  return bufId;
}

void set_major_version(BufMgr* buf, int32_t bufId, uint16_t val) {
  memcpy(buf->bp->slots[bufId].pg + MAJOR_VERSION_BYTE_POS, &val, MAJOR_VERSION_BYTE_SIZE);
}

void set_minor_version(BufMgr* buf, int32_t bufId, uint32_t val) {
  memcpy(buf->bp->slots[bufId].pg + MINOR_VERSION_BYTE_POS, &val, MINOR_VERSION_BYTE_SIZE);
}

void set_patch_num(BufMgr* buf, int32_t bufId, uint32_t val) {
  memcpy(buf->bp->slots[bufId].pg + PATCH_NUM_BYTE_POS, &val, PATCH_NUM_BYTE_SIZE);
}

void set_page_size(BufMgr* buf, int32_t bufId, uint16_t val) {
  memcpy(buf->bp->slots[bufId].pg + PAGE_SIZE_BYTE_POS, &val, PAGE_SIZE_BYTE_SIZE);
}

uint16_t get_major_version(BufMgr* buf) {
  int32_t bufId = get_boot_page_bufid(buf);

  uint16_t majorVersion;

  memcpy(&majorVersion, buf->bp->slots[bufId].pg + MAJOR_VERSION_BYTE_POS, MAJOR_VERSION_BYTE_SIZE);

  return majorVersion;
}

uint32_t get_minor_version(BufMgr* buf) {
  int32_t bufId = get_boot_page_bufid(buf);

  uint32_t minorVersion;

  memcpy(&minorVersion, buf->bp->slots[bufId].pg + MINOR_VERSION_BYTE_POS, MINOR_VERSION_BYTE_SIZE);

  return minorVersion;
}

uint32_t get_patch_num(BufMgr* buf) {
  int32_t bufId = get_boot_page_bufid(buf);

  uint32_t patchNum;

  memcpy(&patchNum, buf->bp->slots[bufId].pg + PATCH_NUM_BYTE_POS, PATCH_NUM_BYTE_SIZE);

  return patchNum;
}

uint16_t get_page_size(BufMgr* buf) {
  int32_t bufId = get_boot_page_bufid(buf);

  uint16_t pageSize;

  memcpy(&pageSize, buf->bp->slots[bufId].pg + PAGE_SIZE_BYTE_POS, PAGE_SIZE_BYTE_SIZE);

  return pageSize;
}


void flush_boot_page(BufMgr* buf) {
  bufpool_flush_page(buf->fdesc->fd, buf->bp, BOOT_PAGE_ID);
}