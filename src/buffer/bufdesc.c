#include <stdlib.h>

#include "buffer/bufdesc.h"

BufDescArr* bufdesc_init(int size) {
  BufDescArr* bd = malloc(sizeof(BufDescArr));
  bd->size = size;
  bd->descArr = calloc(size, sizeof(BufDesc*));

  for (int i = 0; i < size; i++) {
    bd->descArr[i] = malloc(sizeof(BufDesc));
    bd->descArr[i]->tag = malloc(sizeof(BufTag));
    bd->descArr[i]->tag->fileId = 0;
    bd->descArr[i]->tag->pageId = 0;
    bd->descArr[i]->pinCount = 0;
    bd->descArr[i]->useCount = 0;
    bd->descArr[i]->isDirty = false;
    bd->descArr[i]->isValid = true;
  }

  return bd;
}

void bufdesc_destroy(BufDescArr* bd) {
  for (int i = 0; i < bd->size; i++) {
    free(bd->descArr[i]->tag);
    free(bd->descArr[i]);
  }

  free(bd->descArr);
  free(bd);
}

BufTag* bufdesc_new_buftag(uint32_t fileId, int32_t pageId) {
  BufTag* tag = malloc(sizeof(BufTag));
  tag->fileId = fileId;
  tag->pageId = pageId;

  return tag;
}

void bufdesc_free_buftag(BufTag* tag) {
  free(tag);
}

bool bufdesc_is_unused(BufDesc* desc) {
  if (desc->tag->fileId == 0 || desc->tag->pageId == 0) return true;

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

void bufdesc_unpin(BufDesc* desc) {
  desc->pinCount--;
}

void bufdesc_set_tag(BufDesc* desc, BufTag* tag) {
  desc->tag->fileId = tag->fileId;
  desc->tag->pageId = tag->pageId;
}

void bufdesc_set_dirty(BufDesc* desc) {
  desc->isDirty = true;
}

void bufdesc_reset(BufDesc* desc) {
  desc->isDirty = false;
  desc->pinCount = 0;
  desc->useCount = 0;
  desc->tag->fileId = 0;
  desc->tag->pageId = 0;
}

static bool bufdesc_tag_comparison(BufTag* tag1, BufTag* tag2) {
  if (tag1 == NULL || tag2 == NULL) return false;

  if (tag1->fileId == tag2->fileId && tag1->pageId == tag2->pageId) return true;

  return false;
}

int32_t bufdesc_find_slot(BufDescArr* bd, BufTag* tag) {
  for (int i = 0; i < bd->size; i++) {
    if (bufdesc_tag_comparison(bd->descArr[i]->tag, tag)) {
      return i;
    }
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