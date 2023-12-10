#ifndef PAGE_H
#define PAGE_H

#include <stdint.h>

// disabling memory alignment because I don't want to deal with it
#pragma pack(push, 1)
typedef struct PageHeader {
  uint32_t pageId;
  uint8_t pageType;
  uint8_t indexLevel;
  uint32_t prevPageId;
  uint32_t nextPageId;
  uint16_t numRecords;
  uint16_t freeBytes;
  uint16_t freeData;
} PageHeader;

typedef struct SlotPointer {
  uint16_t recordOff;
  uint16_t recordLen;
} SlotPointer;

#endif /* PAGE_H */