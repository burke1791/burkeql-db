#include <stdlib.h>

#include "buffer/bufdesc.h"

BufDescArr* bufdesc_init(int size) {
  BufDescArr* bd = malloc(sizeof(BufDescArr));
  bd->size = size;
  bd->descArr = calloc(size, sizeof(BufDesc*));

  for (int i = 0; i < size; i++) {
    bd->descArr[i] = malloc(sizeof(BufDesc));
    bd->descArr[i]->pageId = 0;
    bd->descArr[i]->pinCount = 0;
    bd->descArr[i]->useCount = 0;
    bd->descArr[i]->isDirty = false;
    bd->descArr[i]->isValid = true;
  }

  return bd;
}

void bufdesc_destroy(BufDescArr* bd) {
  for (int i = 0; i < bd->size; i++) {
    free(bd->descArr[i]);
  }

  free(bd->descArr);
  free(bd);
}

static bool bufdesc_is_unused(BufDesc* desc) {
  if (desc->pageId == 0) return true;

  return false;
}

void bufdesc_start_io(BufDesc* desc) {
  desc->isValid = false;
}

void bufdesc_end_io(BufDesc* desc) {
  desc->isValid = true;
}

void bufdesc_pin(BufDesc* desc) {
  desc->pinCount++;
  desc->useCount++;
}

int32_t bufdesc_find_slot(BufDescArr* bd, uint32_t pageId) {
  for (int i = 0; i < bd->size; i++) {
    if (pageId == bd->descArr[i]->pageId) return i;
  }

  return -1;
}

int32_t bufdesc_find_empty_slot(BufDescArr* bd) {
  for (int i = 0; i < bd->size; i++) {
    if (bufdesc_is_unused(bd->descArr[i])) {
      bufdesc_start_io(bd->descArr[i]);
      return i;
    }
  }

  return -1;
}