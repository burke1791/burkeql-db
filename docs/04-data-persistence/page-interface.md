# DB Page Interface

We're FINALLY to the fun part. Well, at least I think this is the fun part. We get to start writing the code that serves as the backbone of the storage engine: the page interface.

Using the concepts discussed in the [Page Structure](../../03-db-page/page-structure) section, we're going to write the functionality that allocates a new page, maintains header fields, and serializes/deserializes data into table rows.

## Page Header

The first thing we want to do is define a `Page` type. Since a data page is just a fixed-size block of memory, we can use a `char*` as the base data type:

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

The `#pragma pack(push, 1)` line tells the compiler not to align any of the struct properties to the machine word boundary. Disabling memory alignment is a performance penalty, but efficiency is not the goal of this project.

## Slot Array

Next we want to define a struct to represent item pointers in the slot array. Slot pointers are just two 2-byte integers containing the byte-offset from the beginning of the page to its corresponding record, and the byte-length of the record data.

```c
typedef SlotPointer {
  uint16_t offset;
  uint16_t length;
} SlotPointer;
```

## API

Now that we have our structs defined, we can start thinking about functionality we need to expose to the rest of the program. A useful way of doing this is thinking about our program's lifecycle and how it will interact with a data page.

Let's first consider the scenario of a brand new database. We run the program and it attempts to open the data file we specify in the config file. Since the file doesn't exist, we create a brand new, empty file. Then we try to insert some data. Since the file is empty, we need a way to allocate memory for a new data page and return a pointer to that block of memory to the caller.

```c
Page new_page();
void free_page(Page pg);
```

Remember we defined `Page` as a `char*` above, so we're just returning a pointer to our newly allocated block of memory. And because our function is allocating memory, we also need a corresponding cleanup function.

Ok great, our caller is able to get a pointer to the data page. Next, we need to provide a function that can insert data into a page.

```c
bool page_insert(Page pg, char* data, uint16_t length);
```

This function will be responsible for taking the `char* data` record and inserting it into the next available spot in the data page. Along with that, it will also need to create a new `SlotPointer` and prepend it on the slot array, and update the relevant header fields.

Lastly, we need a pair of functions to read and write pages to disk:

```c
Page read_page(FILE* fp, uint32_t pageId);
void flush_page(FILE* fp, Page pg);
```

And that's all we need for basic insert actions. In the next section we'll write the code for each of these three functions, as well as some utility functionality.

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

bool page_insert(Page pg, char* data, uint16_t length);

Page read_page(FILE* fp, uint32_t pageId);
void flush_page(FILE* fp, Page pg);

#endif /* PAGE_H */
```