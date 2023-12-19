# Buffer Pool

Before we go too far in implementing a simple query engine, we need to refactor how our program is able to interact with pages on disk. Right now, we only allow a single data page, so it's easy to pull it into memory at the beginning of the program, pass it around to any systems that need it, then flush to disk at the end.

This won't work when we support multiple pages, and by then we'll have written a lot more code that works with pages so refactoring is best done early. We're going to create a mini buffer pool.

## What is a Buffer Pool?

Relational databases are built to store and work with huge amounts of data. In such situations, the amound of data they store will far exceed the amount of memory they're given by the host machine. So in order to read and write to a lot of data pages while working within its memory constraints, they keep data pages in something called a buffer pool.

A buffer pool is a shared pool of memory that keeps track of all data pages currently in memory. If some process needs to read or write from a specific data page, it will ask the buffer pool for the page. If the page is currently in memory, the buffer pool returns a pointer to the data page quick and easy. If the page is not currently in memory, the buffer pool must first determine if it has enough space to house a new page. If there is enough space, then it'll pull the page from disk into memory and return a pointer to the page. If the buffer pool does not have enough space, it need to choose a page to evict from the pool before it can read the requested page into memory.

At present, our needs are not that complex, so we can get away with writing a really dumb buffer pool. But it is a solid starting point, and a good way to keep data file/page access constrained to a single subsystem. 

## Buffer Pool Header

`src/include/buffer/bufpool.h`

```c
typedef struct BufPoolSlot {
  uint32_t pageId;
  Page pg;
} BufPoolSlot;

typedef struct BufPool {
  FileDesc* fdesc;
  int size;
  BufPoolSlot* slots;
} BufPool;
```

First we need two structs: one for the buffer pool itself and another for the page slots in the buffer pool. The `BufPool` struct will be responsible for reading and writing data for a single file only, hence the `FileDesc` pointer we store in it. It also has a fixed size, which corresponds to the number of slots we allow in the buffer pool - the `slots` property is an array of `BufPoolSlot`s.

The `BufPoolSlot` struct represents a single page in memory. When occupied, we set the `pageId` to whatever page is currently in the slot. If the slot is empty, we set `pageId` to 0.

```c
BufPool* bufpool_init(FileDesc* fdesc, int numSlots);
void bufpool_destroy(BufPool* bp);

BufPoolSlot* bufpool_read_page(BufPool* bp, uint32_t pageId);
void bufpool_flush_page(BufPool* bp, uint32_t pageId);
void bufpool_flush_all(BufPool* bp);
```

Now our functions, starting with the allocate/free pair. The allocator takes a `numSlots` parameter which enforces a maximum size of the buffer pool on initialization. This is kind of a hacky way we're using to limit memory consumption of our database. In reality, the rest of the program can consume as much as it wants, we're just restricting the buffer pool.

The remaining three functions are replacements for the read/write functions we already have. But we also create a `flush_all` function that our program will call when it receives the `\quit` command.

## Buffer Pool Implementation

`src/buffer/bufpool.c`

```c
extern Config* conf;

BufPool* bufpool_init(FileDesc* fdesc, int numSlots) {
  BufPool* bp = malloc(sizeof(BufPool));
  bp->fdesc = fdesc;
  bp->size = numSlots;
  bp->slots = calloc(numSlots, sizeof(BufPoolSlot));

  // initialize each slot with a blank page
  for (int i = 0; i < numSlots; i++) {
    bp->slots[i].pg = new_page();
    bp->slots[i].pageId = 0;
  }

  return bp;
}

void bufpool_destroy(BufPool* bp) {
  if (bp == NULL) return;

  if (bp->slots != NULL) {
    for (int i = 0; i < bp->size; i++) {
      if (bp->slots[i].pg != NULL) free_page(bp->slots[i].pg);
    }

    free(bp->slots);
  }

  free(bp);
}
```

Starting with the allocator/free functions again. The allocator sets the `fdesc` and `size` properties according to its input parameters, then initializes the `slots` array with a number equal to `numSlots`. Then we need to initialize each of the slots with blank database pages that are available for an incoming data page being read from disk.

The free function, as you'd expect, loops through the slots and frees everything we allocated in the beginning.

```c
BufPoolSlot* bufpool_read_page(BufPool* bp, uint32_t pageId) {
  BufPoolSlot* slot = bufpool_find_empty_slot(bp);

  if (slot == NULL) {
    // evict a page and return the empty slot
    slot = bufpool_evict_page(bp);
  }

  lseek(bp->fdesc->fd, (pageId - 1) * conf->pageSize, SEEK_SET);
  int bytes_read = read(bp->fdesc->fd, slot->pg, conf->pageSize);

  if (bytes_read != conf->pageSize) {
    printf("Bytes read: %d\n", bytes_read);
    
    // pageId doesn't exist on disk, so we create it
    PageHeader* pgHdr = (PageHeader*)slot->pg;
    pgHdr->pageId = pageId;
    pgHdr->freeBytes = conf->pageSize - sizeof(PageHeader);
    pgHdr->freeData = conf->pageSize - sizeof(PageHeader);
  }

  slot->pageId = pageId;

  return slot;
}
```

The `read_page` function has more going on than you might expect. It first needs to find an unclaimed slot, which we wrote a static helper function for (see below). If all slots are occupied, we need to evict a page by flushing it to disk - using another static helper function. After that, we'll have an empty slot we can use to read a page into. The rest is essentially the same as the old `read_page` function.

```c
static BufPoolSlot* bufpool_find_empty_slot(BufPool* bp) {
  for (int i = 0; i < bp->size; i++) {
    if (bp->slots[i].pageId == 0) return &bp->slots[i];
  }

  return NULL;
}

static BufPoolSlot* bufpool_evict_page(BufPool* bp) {
  // just evict the first page for now
  uint32_t pageId = bp->slots[0].pageId;
  bufpool_flush_page(bp, pageId);
  bp->slots[0].pageId = 0;
  return &bp->slots[0];
}
```

These helper functions are pretty basic. In the first one, we simply loop through the `slots` array and return the first `pageId = 0` slot that we find - `NULL` otherwise. The `evict_page` function is extremely simple right now - we're just evicting the first page in the `slots` array. When we start supporting multiple pages, we'll come up with a better way to decide which page gets flushed.

```c
void bufpool_flush_page(BufPool* bp, uint32_t pageId) {
  BufPoolSlot* slot = bufpool_find_slot_by_page_id(bp, pageId);

  if (slot == NULL) {
    printf("Page does not exist in the buffer pool\n");
    return;
  }

  if (!flush_page(bp->fdesc->fd, slot->pg, pageId)) {
    printf("Flush page failure\n");
    return;
  }

  slot->pageId = 0;
}
```

Next up, we have our flush page function. Given a `pageId`, it finds the slot where that page exists, then calls `flush_page` to write it to disk.

```c
static BufPoolSlot* bufpool_find_slot_by_page_id(BufPool* bp, uint32_t pageId) {
  for (int i = 0; i < bp->size; i++) {
    if (bp->slots[i].pageId == pageId) return &bp->slots[i];
  }

  return NULL;
}

static bool flush_page(int fd, Page pg, uint32_t pageId) {
  uint32_t headerPageId = ((PageHeader*)pg)->pageId;

  if (headerPageId != pageId || pageId == 0) {
    printf("Page Ids do not match\n");
    return false;
  }

  lseek(fd, (pageId - 1) * conf->pageSize, SEEK_SET);
  int bytes_written = write(fd, pg, conf->pageSize);

  if (bytes_written != conf->pageSize) {
    printf("Bytes written: %d\n", bytes_written);
    return false;
  }

  return true;
}
```

Finding a slot by pageId is pretty simple - just loop through the slots until you find the matching pageId. The `flush_page` function is essentially the same as the one we're replacing. Move the file offset to the correct spot, then attempt to write `conf->pageSize` bytes to the file.

```c
void bufpool_flush_all(BufPool* bp) {
  for (int i = 0; i < bp->size; i++) {
    BufPoolSlot* slot = &bp->slots[i];
    flush_page(bp->fdesc->fd, slot->pg, slot->pageId);
  }
}
```

Finally, we have our `flush_all` function. Simple loop through the `slots` array and calling `flush_page`.

## Removing Old Code

`src/include/storage/page.h`

```diff
-Page read_page(int fd, uint32_t pageId);
-void flush_page(int fd, Page pg);
```

We no longer need the read and flush functions we wrote in our storage engine code.

`src/storage/page.c`

```diff
-Page read_page(int fd, uint32_t pageId) {
-  Page pg = new_page();
-  lseek(fd, (pageId - 1) * conf->pageSize, SEEK_SET);
-  int bytes_read = read(fd, pg, conf->pageSize);
-
-  if (bytes_read != conf->pageSize) {
-    printf("Bytes read: %d\n", bytes_read);
-    PageHeader* pgHdr = (PageHeader*)pg;
-    /* Since this is a brand new page, we need to set the header fields appropriately */
-    pgHdr->pageId = pageId;
-    pgHdr->freeBytes = conf->pageSize - sizeof(PageHeader);
-    pgHdr->freeData = conf->pageSize - sizeof(PageHeader);
-  }
-
-  return pg;
-}
-
-void flush_page(int fd, Page pg) {
-  int pageId = ((PageHeader*)pg)->pageId;
-  lseek(fd, (pageId - 1) * conf->pageSize, SEEK_SET);
-  int bytes_written = write(fd, pg, conf->pageSize);
-
-  if (bytes_written != conf->pageSize) {
-    printf("Page flush unsuccessful\n");
-  }
-}
```

## Updating Main

`src/main.c`

```diff
 /* TEMPORARY CODE SECTION */
 
 #define RECORD_LEN  36  // 12-byte header + 4-byte Int + 20-byte Char(20)
+#define BUFPOOL_SLOTS  1
```

First we add a new macro so we can control how many buffer pool slots we want to allow. Start with one for now since our database can only work with one page anyways.

```diff
-static bool insert_record(Page pg, int32_t person_id, char* name) {
+static bool insert_record(BufPool* bp, int32_t person_id, char* name) {
+  BufPoolSlot* slot = bufpool_read_page(bp, 1);
   RecordDescriptor* rd = construct_record_descriptor();
   Record r = record_init(RECORD_LEN);
   serialize_data(rd, r, person_id, name);
-  bool insertSuccessful = page_insert(pg, r, RECORD_LEN);
+  bool insertSuccessful = page_insert(slot->pg, r, RECORD_LEN);
 
   free_record_desc(rd);
   free(r);
   
   return insertSuccessful;
 }
```

Instead of passing a `Page` around everywhere, we're going to start passing around a pointer to the `BufPool`.

```diff
 int main(int argc, char** argv) {
   // initialize global config
   conf = new_config();
 
   if (!set_global_config(conf)) {
     return EXIT_FAILURE;
   }
 
   // print config
   print_config(conf);
 
   FileDesc* fdesc = file_open(conf->dataFile);
-  Page pg = read_page(fdesc->fd, 1);
+  BufPool* bp = bufpool_init(fdesc, BUFPOOL_SLOTS);
 
   while(true) {
     print_prompt();
     Node* n = parse_sql();
 
     if (n == NULL) continue;
 
     print_node(n);
 
     switch (n->type) {
       case T_SysCmd:
         if (strcmp(((SysCmd*)n)->cmd, "quit") == 0) {
           free_node(n);
           printf("Shutting down...\n");
-          flush_page(fdesc->fd, pg);
-          free_page(pg);
+          bufpool_flush_all(bp);
+          bufpool_destroy(bp);
           file_close(fdesc);
           return EXIT_SUCCESS;
         }
         break;
       case T_InsertStmt:
         int32_t person_id = ((InsertStmt*)n)->personId;
         char* name = ((InsertStmt*)n)->name;
-        if (!insert_record(pg, person_id, name)) {
+        if (!insert_record(bp, person_id, name)) {
           printf("Unable to insert record\n");
         }
     }
 
     free_node(n);
   }
 
   return EXIT_SUCCESS;
 }
```

First thought here: our main function is getting too big, we'll need to fix that. Second, we just have a few updates to make sure we're passing around a `BufPool` pointer instead of a `Page`.

`src/Makefile`

```diff
SRC_FILES = main.c \
			parser/parse.c \
			parser/parsetree.c \
			global/config.c \
			storage/file.c \
			storage/page.c \
			storage/record.c \
+   		storage/datum.c \
+   		buffer/bufpool.c
```

The program should behave exactly the same as it did before.