/**
 * @file bufdesc.h
 * @author Chris Burke
 * @brief 
 * @version 0.1
 * @date 2024-06-04
 * 
 * @copyright Copyright (c) 2024
 * 
 */

#ifndef BUFDESC_H
#define BUFDESC_H

#include <stdint.h>
#include <stdbool.h>

typedef struct BufTag {
  uint32_t fileId;
  int32_t pageId;
} BufTag;

#pragma pack(push, 1) /* disabling memory alignment because I don't want to deal with it */
typedef struct BufDesc {
  BufTag* tag;
  int pinCount;   /* number of processes currently accessing the page */
  int useCount;   /* number of times the page has been accessed since being loaded into memory */
  bool isDirty;   /* page contents have changed since it was loaded from disk */
  bool isValid;   /* whether or not the page can be accessed. Writers set this to false.
                     It is also set to false when an IO is in progress. */
} BufDesc;

// size is stored in BufMgr->size
typedef struct BufDescArr {
  int size;
  BufDesc** descArr;
} BufDescArr;

BufDescArr* bufdesc_init(int size);
void bufdesc_destroy(BufDescArr* bd);

BufTag* bufdesc_new_buftag(uint32_t fileId, int32_t pageId);
void bufdesc_free_buftag(BufTag* tag);

void bufdesc_start_io(BufDesc* desc);
void bufdesc_end_io(BufDesc* desc);
void bufdesc_pin(BufDesc* desc);

void bufdesc_set_tag(BufDesc* desc, BufTag* tag);

int32_t bufdesc_find_slot(BufDescArr* bd, BufTag* tag);
int32_t bufdesc_find_empty_slot(BufDescArr* bd);

#endif /* BUFDESC_H */