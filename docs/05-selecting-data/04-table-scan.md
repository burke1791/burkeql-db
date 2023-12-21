# Table Scan

We're able to insert data into our database, but that's effectively useless if we don't have any way to read it back out. Our parser can handle basic select statements, so now we need to write the code that reads through a table's data pages and returns an organized list or row/column data.

We're going to start out super basic and implement a full table scan. That's where our engine starts at the data page and reads all data rows until it runs out of pages. So what information does our table scan function need to do its job?

For starters, it needs a pointer to the `BufPool` so that it can request data pages be read into memory. It will also need a `RecordDescriptor` about the table its scanning so that it can correctly parse the bytes stored in each row. And lastly, it needs somewhere to dump the parsed data - we'll be using a doubly linked list.

Fair warning: we're going to be writing a lot of code and it's going to be all over the place.

## Table Access Method

With this new table scan functionality, I am starting a new folder for files containing "access" methods. I.e. since we're scanning a table is a way of "accessing" its data, so the code belongs to the "access" section of the codebase.

`src/include/access/tableam.h`

```c
#include "storage/table.h"
#include "buffer/bufpool.h"
#include "utility/linkedlist.h"

void tableam_fullscan(BufPool* bp, TableDesc* td, LinkedList* rows);
```

Pretty basic stuff here. No structs, just a single function for now. As mentioned above, our table scan method needs access to the buffer pool, metadata about the table and its contents (`TableDesc`), and a landing zone for the data it reads from the data pages.

Now, what the hell is a `TableDesc`?

`src/include/storage/table.h`

```c
#include "storage/record.h"

typedef struct TableDesc {
  char* tablename;
  RecordDescriptor* rd;
} TableDesc;

TableDesc* new_tabledesc(char* tablename);

void free_tabledesc(TableDesc* td);
```

It's just a `RecordDescriptor` that also contains the table's name. In this header file, we also define the standard allocator/free pair of functions. Let's get them out of the way real quick:

`src/storage/table.c`

```c
TableDesc* new_tabledesc(char* tablename) {
  TableDesc* td = malloc(sizeof(TableDesc));
  td->tablename = tablename;
  td->rd = NULL;

  return td;
}

void free_tabledesc(TableDesc* td) {
  if (td == NULL) return;

  // if (td->tablename != NULL) free(td->tablename);
  if (td->rd != NULL) free_record_desc(td->rd);
  free(td);
}
```

Again, these are pretty standard memory management functions. Right now I have that line commented out in the `free_tabledesc` function because our table is hard-coded. Meaning when we create a new `TableDesc`, the `tablename` parameter is hard-coded in our source code, i.e. not `malloc`'d, so there is nothing to free. Later on, when we support more than one table, this will get uncommented.

Alright, back to the table scan function. The third, and last, parameter is a `LinkedList`. Let's check it out:

`src/include/utility/linkedlist.h`

```c
#pragma pack(push, 1) /* disabling memory alignment because I don't want to deal with it */
typedef struct ListItem {
  void* ptr;
  struct ListItem* prev;
  struct ListItem* next;
} ListItem;

typedef struct LinkedList {
  int numItems;
  ListItem* head;
  ListItem* tail;
} LinkedList;

LinkedList* new_linkedlist();
void free_linkedlist(LinkedList* l);

void linkedlist_append(LinkedList* l, void* ptr);
```

This is a standard linked list implementation. We store previous and next pointers in each `ListItem`, and the head and tail of the list in the `LinkedList` struct. We also have the allocator and free functions, along with an append function. We currently don't need a `delete_item` function, so I didn't write one. And I am disabling memory alignment on the `ListItem` because we store `Datum` arrays in this list and I got a lot of read access errors. The `#pragma` here gets rid of those. A reminder, I am not an expert at C - just stupid enough to get something working.

Anyways, let's look at how the linked list is implemented:

`src/utility/linkedlist.c`

```c
LinkedList* new_linkedlist() {
  LinkedList* l = malloc(sizeof(LinkedList));
  l->numItems = 0;
  l->head = NULL;
  l->tail = NULL;

  return l;
}

void free_linkedlist(LinkedList* l) {
  if (l == NULL) return;

  if (l->head != NULL) {
    ListItem* li = l->head;
    while (li != NULL) {
      ListItem* curr = li;
      li = curr->next;
      free(curr);
    }
  }

  free(l);
}

void linkedlist_append(LinkedList* l, void* ptr) {
  if (l == NULL) {
    printf("Error: tried to append to a NULL LinkedList");
    return;
  }

  ListItem* new = malloc(sizeof(ListItem));
  new->ptr = ptr;
  new->next = NULL;
  new->prev = NULL;

  if (l->numItems == 0) {
    l->head = new;
  } else {
    ListItem* tail = l->tail;
    tail->next = new;
    new->prev = tail;
  }

  l->tail = new;
  l->numItems++;
}
```

The allocator is pretty basic - just initializing its properties. The free function loops through the entire list and frees the memory it has consumed. **Super Important Note**: since this is a generic linked list, the caller is responsible for freeing whatever is stored in the `ListItem->ptr` memory blocks before calling `free_linkedlist`.

The append function is also pretty standard. We just create a new `ListItem` and add it to the end of the list, updating the linking chain as needed.

Okay, that takes care of everything we need to know about the inputs to our table scan function. Now let's take a look at the function itself.

## Table Scan Function

`src/access/tableam.c`

```c
extern Config* conf;

void tableam_fullscan(BufPool* bp, TableDesc* td, LinkedList* rows) {
  BufPoolSlot* slot = bufpool_read_page(bp, 1);

  while (slot != NULL) {
    PageHeader* pgHdr = (PageHeader*)slot->pg;
    int numRecords = pgHdr->numRecords;

    for (int i = 0; i < numRecords; i++) {
      RecordSetRow* row = new_recordset_row(td->rd->ncols);
      int slotPointerOffset = conf->pageSize - (sizeof(SlotPointer) * (i + 1));
      SlotPointer* sp = (SlotPointer*)(slot->pg + slotPointerOffset);
      defill_record(td->rd, slot->pg + sp->offset, row->values);
      linkedlist_append(rows, row);
    }

    slot = bufpool_read_page(bp, pgHdr->nextPageId);
  }
}
```

The basic workflow of a table scan is as follows:

1. Request the first page of the table from the buffer pool
1. Determine the number or records on the page from the header field `numRecords`
1. Loop through the slot array, finding the location of each record
1. Extract the data from each record (`defill_record`) into a `Datum` array
1. Append the `Datum` array to the `rows` linked list
1. Repeat until we hit the last page in the table

At the top, we request `pageId` 1 from the buffer pool - this is hard-coded right now, but will change when we support multiple tables. Then we enter a loop that continues until we hit the end of the data pages for our table.

Inside the `while` loop, we grab the number of records from the header, which is also how many slot pointers there are in the slot array. From here, we loop through each record and extract each column into a `Datum` array via `defill_record` and then append it to our results linked list.

### RecordSet

Let's dive in to some of the new code, starting with the new `RecordSetRow` data type:

`src/include/resultset/recordset.h`

```c
#include "storage/record.h"
#include "utility/linkedlist.h"

typedef struct RecordSetRow {
  Datum* values;
} RecordSetRow;

typedef struct RecordSet {
  LinkedList* rows;
} RecordSet;

RecordSet* new_recordset();
void free_recordset(RecordSet* rs, RecordDescriptor* rd);

RecordSetRow* new_recordset_row(int ncols);
void free_recordset_row(RecordSetRow* row, RecordDescriptor* rd);
```

Two structs, and their associated allocator/free functions. Right now, `RecordSet` might seem like a pointless wrapper around a `LinkedList`, but it will become more complex and necessary as our program evolves. Same thing goes for the `RecordSetRow`. And as you can probably guess, the linked list in `RecordSet` will be filled with `RecordSetRow` items as we scan the table data.

The free functions here need to take a `RecordDescriptor` as input because they'll be responsible for freeing each `Datum` inside the `Datum` array. And in order to do so, we need to know what kinds of data are stored in the `Datum` array.

`src/resultset/recordset.c`

```c
RecordSet* new_recordset() {
  RecordSet* rs = malloc(sizeof(RecordSet));
  rs->rows = NULL;

  return rs;
}

static void free_recordset_row_columns(RecordSetRow* row, RecordDescriptor* rd) {
  for (int i = 0; i < rd->ncols; i++) {
    if (rd->cols[i].dataType == DT_CHAR) {
      free((void*)row->values[i]);
    }
  }
}

static void free_recordset_row_list(LinkedList* rows, RecordDescriptor* rd) {
  ListItem* li = rows->head;

  while (li != NULL) {
    free_recordset_row((RecordSetRow*)li->ptr, rd);
    li = li->next;
  }
}

void free_recordset(RecordSet* rs, RecordDescriptor* rd) {
  if (rs == NULL) return;

  if (rs->rows != NULL) {
    free_recordset_row_list(rs->rows, rd);
    free_linkedlist(rs->rows);
  }

  free(rs);
}

RecordSetRow* new_recordset_row(int ncols) {
  RecordSetRow* row = malloc(sizeof(RecordSetRow));
  row->values = malloc(ncols * sizeof(Datum));
  return row;
}

void free_recordset_row(RecordSetRow* row, RecordDescriptor* rd) {
  if (row->values != NULL) {
    free_recordset_row_columns(row, rd);
    free(row->values);
  }
  free(row);
}
```

And here's the code for our `RecordSet` structs. Nothing too complex is going on here - just allocating memory and freeing memory as needed. The only noteworthy function is `free_recordset_row_columns`. This is where we use the information stored in the `RecordDescriptor` to free data inside the `Datum` array. Basic types like ints do not need to be free'd, but `char*`'s do. So we loop through the column metadata and call `free` on any `DT_CHAR` data types in the `Datum` array.

### defill_record

Now, let's continue along our table scan function. Right now we're inside the `for` loop and we've just allocated a new `RecordSetRow`. Here's the relevant section for reference:

`src/access/tableam.c` (`tableam_fullscan()`)

```c
    for (int i = 0; i < numRecords; i++) {
      RecordSetRow* row = new_recordset_row(td->rd->ncols);
      int slotPointerOffset = conf->pageSize - (sizeof(SlotPointer) * (i + 1));
      SlotPointer* sp = (SlotPointer*)(slot->pg + slotPointerOffset);
      defill_record(td->rd, slot->pg + sp->offset, row->values);
      linkedlist_append(rows, row);
    }
```

Our next step is to grab the next `SlotPointer`. Remember, the slot array grows from the end of the page towards the beginning. So, the first slot pointer is the very last 4 bytes on the page, the next one is the 4 bytes preceeding it, and so on. We can get the byte position by using this formula: `conf->pageSize - (sizeof(SlotPointer) * (i + 1))`. Once we have its location, we can create a `SlotPointer*` object for easy reference.

Now, we call the workhorse of our table scan method. `defill_record` uses the information in the `RecordDescriptor` and marches through the bytes where the data record are stored and extracts them into the `Datum` array we give to it.

We define `defill_record` in the same place we defined `fill_record`.

`src/include/storage/record.h`

```diff
 void construct_column_desc(Column* col, char* colname, DataType type, int colnum, int len);
 
 void fill_record(RecordDescriptor* rd, Record r, Datum* data);
+void defill_record(RecordDescriptor* rd, Record r, Datum* values);
 
 #endif /* RECORD_H */
```

And here's the code:

`src/storage/record.c`

```diff
+void defill_record(RecordDescriptor* rd, Record r, Datum* values) {
+  int offset = sizeof(RecordHeader);
+  for (int i = 0; i < rd->ncols; i++) {
+    Column* col = &rd->cols[i];
+    values[i] = record_get_col_value(col, r, &offset);
+  }
+}
```

Pretty simple, right? We loop through the columns in our `RecordDescriptor` and extract each column's value, via a static helper function, into the `Datum` array. Note that we're passing `&offset` into the helper function. We want `offset` to change as we progress through the loop, so we need to pass in the address of the variable if we want the inner function to be able to perform persistent operations on its value.

I should mention right now we COULD just perform `offset += col->len;` at the end of the for loop and be good. However, this method will not work when we get to variable-length columns. The `Column` object in the `RecordDescriptor` won't be able to store those lengths because they could be different for each row. The only way to find the length of those columns is to dig further into the `Record` data, which happens inside these helper functions. Thus, we make them responsible for incrementing `offset`.

`src/storage/record.c`

```diff
+static Datum record_get_col_value(Column* col, Record r, int* offset) {
+  switch (col->dataType) {
+    case DT_INT:
+      return record_get_int(r, offset);
+    case DT_CHAR:
+      return record_get_char(r, offset, col->len);
+    default:
+      printf("record_get_col_value() | Unknown data type!\n");
+      return (Datum)NULL;
+  }
+}
```

This function is a wrapper around type-specific data extraction. As you can imagine, we'll be adding more case statements to this function as we expand our list of supported data types.

`src/storage/record.c`

```diff
+static Datum record_get_int(Record r, int* offset) {
+  int32_t intVal;
+  memcpy(&intVal, r + *offset, 4);
+  *offset += 4;
+  return int32GetDatum(intVal);
+}
+
+static Datum record_get_char(Record r, int* offset, int charLen) {
+  char* pChar = malloc(charLen + 1);
+  memcpy(pChar, r + *offset, charLen);
+  pChar[charLen] = '\0';
+  *offset += charLen;
+  return charGetDatum(pChar);
+}
```

And here is where we do the actual data extraction. We create a variable of the correct data type, then `memcpy` the contents of the `Record` into that variable. Increment `offset` by dereferencing it, then return the `Datum` representation of the value.

We have additional complexity for the `record_get_char` function. When we insert `DT_CHAR` data, we only store the relevant characters - we DO NOT store the null terminator. So when we extract the value, we need to `malloc(charLen + 1)` to make sure there's enough memory to append the null terminator to the end of the string.

And that's all the code we need to perform a table scan on our table. Unfortunately, it's not enough to get our program to do anything useful just yet. I feel like this section has gone on long enough, so we'll round out the whole process in the next section.