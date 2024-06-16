#ifndef BOOT_H
#define BOOT_H

#include <stdint.h>

#include "buffer/bufmgr.h"

/**
 * This file defines the API spec for getting and setting
 * data on the database's boot page. The boot page is always
 * at pageId=1 and is only a single page.
 * 
 * The fields stored in the boot page are as follows:
 * 
 * Field                    Byte Pos      Byte Size
 * ------------------------------------------------
 * major_version            0             2
 * minor_version            2             4
 * patch_num                6             4
 * page_size                10            2
 */

#define BOOT_PAGE_ID 1

/**
 * hard-coding these values for now
 */
#define MAJOR_VERSION 1
#define MINOR_VERSION 2
#define PATCH_NUM 69

#define MAJOR_VERSION_BYTE_POS 0
#define MINOR_VERSION_BYTE_POS 2
#define PATCH_NUM_BYTE_POS 6
#define PAGE_SIZE_BYTE_POS 10

#define MAJOR_VERSION_BYTE_SIZE 2
#define MINOR_VERSION_BYTE_SIZE 4
#define PATCH_NUM_BYTE_SIZE 4
#define PAGE_SIZE_BYTE_SIZE 2

bool init_boot_page(BufMgr* buf, BufTag* tag);

/**
 * These setter functions should ONLY be called at the
 * beginning of an initdb operation.
 * 
 * Changing boot_page values after a database has been
 * initialized will break functionality and put the DB
 * in an unusable state
*/
void set_major_version(BufMgr* buf, int32_t bufId, uint16_t val);
void set_minor_version(BufMgr* buf, int32_t bufId, uint32_t val);
void set_patch_num(BufMgr* buf, int32_t bufId, uint32_t val);
void set_page_size(BufMgr* buf, int32_t bufId, uint16_t val);

uint16_t get_major_version(BufMgr* buf);
uint32_t get_minor_version(BufMgr* buf);
uint32_t get_patch_num(BufMgr* buf);
uint16_t get_page_size(BufMgr* buf);

void flush_boot_page(BufMgr* buf);

#endif /* BOOT_H */