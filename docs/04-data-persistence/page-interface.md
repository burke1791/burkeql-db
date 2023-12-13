# DB Page Interface

We're FINALLY to the fun part. Well, at least I think this is the fun part. We get to start writing the code that serves as the backbone of the storage engine: the page interface.

Using the concepts discussed in the [Page Structure](../../03-db-page/page-structure) section, we're going to write the functionality that allocates a new page, maintains header fields, and reads/writes a page from/to disk.

## Page Header

The first thing we want to do is define a `Page` type. Since a data page is just a fixed-size block of memory, we can use a `char*` as the base data type. This isn't super necessary, but it will be helpful as the codebase grows to define function parameters as a `Page` instead of the ambiguous `char*`.

`src/include/storage/page.h`

```c
typedef char* Page;
```

Remember from the concepts section, each data page has a fixed 20-byte header. We can model that with a struct as such:

```c
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
```

The `#pragma pack(push, 1)` line tells the compiler not to align any of the struct properties to the machine word boundary. Disabling memory alignment comes at a performance penalty, but efficiency is not the goal of this project.

## Slot Array

Next we want to define a struct to represent item pointers in the slot array. Slot pointers are just a pair of 2-byte integers containing the byte-offset from the beginning of the page to its corresponding record, and the byte-length of the record data.

```c
typedef SlotPointer {
  uint16_t offset;
  uint16_t length;
} SlotPointer;
```

## API

Now that we have our structs defined, we can start thinking about functionality we need to expose to the rest of the program. A useful way of doing this is thinking about our program's lifecycle and how it will interact with a data page.

To start, let's narrow the scope to a database with a data file that contains only a single page. When the program starts, it will open the file and attempt to read the page into memory. If the file is empty, we create a brand new page. Then our program will flush that page to disk and close the file. Right now we aren't going to worry about storing data records.

First up are a pair of functions responsible for reading and writing data pages to disk:

```c
Page read_page(FILE* fp, uint32_t pageId);
void flush_page(FILE* fp, Page pg);
```

It's a fairly simple task, the caller just needs to have the file pointer readily available. And remember we defined `Page` as a `char*` above, so we're just passing a pointer to the block of memory containing our data page.

Next up, we need a way to allocate memory for the data page and a matching function that frees the memory when we're done with it.

```c
Page new_page();
void free_page(Page pg);
```

And that's all we need for basic disk read/write actions. In the next section we'll write the code for each of these functions, as well as some utility functionality.

## Full File

`src/include/storage/page.h`

```c
#ifndef PAGE_H
#define PAGE_H

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

typedef char* Page;

#pragma pack(push, 1) /* disabling memory alignment because I don't want to deal with it */
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
  uint16_t offset;
  uint16_t length;
} SlotPointer;

Page new_page();
void free_page(Page pg);

Page read_page(FILE* fp, uint32_t pageId);
void flush_page(FILE* fp, Page pg);

#endif /* PAGE_H */
```