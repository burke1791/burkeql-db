#include "system/initdb.h"
#include "global/config.h"
#include "system/boot.h"
#include "system/sysobject.h"

extern Config* conf;

static bool init_obj_objects(BufMgr* buf, BufTag* tag) {
  SysObject* obj = malloc(sizeof(SysObject));
  obj->objectId = 1;
  obj->name = "_objects";
  obj->type = "t";

  
}

/**
 * @brief initializes the database boot and system pages if necessary
 * 
 * @todo eventually use this process to read the first 128-byte chunk of the file
 * and set conf->pageSize from the boot page value. Everything after this will read
 * data in `pageSize` chunks
 * 
 * @param buf 
 */
bool initdb(BufMgr* buf) {
  BufTag* tag = malloc(sizeof(BufTag));
  tag->fileId = FILE_DATA;
  tag->pageId = BOOT_PAGE_ID;

  int32_t bufId = bufmgr_request_bufId(buf, tag);

  if (bufId >= 0) {
    // if boot page is already populated, we can return early
    if (get_major_version(buf) > 0) {
      return true;
    }
  }

  if (!init_boot_page(buf, tag)) {
    printf("Unable to initialize the boot page\n");
    return false;
  }

  if (!init_table_tables(buf, tag)) {
    printf("Unable to initialize the _tables system table\n");
    return false;
  }

  return true;
}