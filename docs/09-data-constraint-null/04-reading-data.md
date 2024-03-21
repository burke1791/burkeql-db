# Reading Data

For the reading data changes, we again follow the code from the top and make updates as needed. Starting at the entry point for a `Select` statement, here's the top-level code block we're going to follow:

```c
case T_SelectStmt:
  if (!analyze_node(n)) {
    printf("Semantic analysis failed\n");
  } else {
    TableDesc* td = new_tabledesc("person");
    td->rd = construct_record_descriptor();
    RecordSet* rs = new_recordset();
    rs->rows = new_linkedlist();
    RecordDescriptor* targets = construct_record_descriptor_from_target_list(((SelectStmt*)n)->targetList);
    
    tableam_fullscan(bp, td, rs->rows);
    resultset_print(td->rd, rs, targets);

    free_recordset(rs, td->rd);
    free_tabledesc(td);
    free_record_desc(targets);
  }
  break;
```

## RecordDescriptor

The line:

```c
    td->rd = construct_record_descriptor();
```

is where our first change happens. We need to add nullability flags to the `RecordDescriptor` and `Column` structs. These flags will allow our output code to skip a bunch of logic checks for cases when the column value is `Not Null`.

`src/include/storage/record.h`

```diff
 #include <stdint.h>
+#include <stdbool.h>
 
 #include "storage/datum.h"

*** omitted for brevity ***

 #pragma pack(push, 1) /* disabling memory alignment because I don't want to deal with it */
 typedef struct Column {
   char* colname;
   DataType dataType;
   int colnum;     /* 0-based col index */
   int len;
+  bool isNotNull;
 } Column;

*** omitted for brevity ***

 typedef struct RecordDescriptor {
   int ncols;        /* number of columns (defined by the Create Table DDL) */
   int nfixed;       /* number of fixed-length columns */
+  bool hasNullableColumns;
   Column cols[];
 } RecordDescriptor;

 Record record_init(uint16_t recordLen);
 void free_record(Record r);
 
 void free_record_desc(RecordDescriptor* rd);
 
-void construct_column_desc(Column* col, char* colname, DataType type, int colnum, int len);
+void construct_column_desc(Column* col, char* colname, DataType type, int colnum, int len, bool  isNotNull);
```

Two simple changes going on here. We add nullability flags to both the `Column` and `RecordDescriptor` structs, and we add a new parameter to the column constructor function.

`src/storage/record.c`

```diff
-void construct_column_desc(Column* col, char* colname, DataType type, int colnum, int len) {
+void construct_column_desc(Column* col, char* colname, DataType type, int colnum, int len, bool  isNotNull) {
   col->colname = strdup(colname);
   col->dataType = type;
   col->colnum = colnum;
   col->len = len;
+  col->isNotNull = isNotNull;
 }
```

We just need to add a new line to set the `isNotNull` property of the `Column` struct.

`src/main.c`

```diff
 static RecordDescriptor* construct_record_descriptor() {
   RecordDescriptor* rd = malloc(sizeof(RecordDescriptor) + (4 * sizeof(Column)));
   rd->ncols = 4;
   rd->nfixed = 2;
 
-  construct_column_desc(&rd->cols[0], "person_id", DT_INT, 0, 4);
-  construct_column_desc(&rd->cols[1], "first_name", DT_VARCHAR, 1, 20);
-  construct_column_desc(&rd->cols[2], "last_name", DT_VARCHAR, 2, 20);
-  construct_column_desc(&rd->cols[3], "age", DT_INT, 3, 4);
+  construct_column_desc(&rd->cols[0], "person_id", DT_INT, 0, 4, true);
+  construct_column_desc(&rd->cols[1], "first_name", DT_VARCHAR, 1, 20, false);
+  construct_column_desc(&rd->cols[2], "last_name", DT_VARCHAR, 2, 20, true);
+  construct_column_desc(&rd->cols[3], "age", DT_INT, 3, 4, false);
+
+  rd->hasNullableColumns = true;
 
   return rd;
 }

 static RecordDescriptor* construct_record_descriptor_from_target_list(ParseList* targetList) {
   RecordDescriptor* rd = malloc(sizeof(RecordDescriptor) + (targetList->length * sizeof (Column)));
   rd->ncols = targetList->length;
 
   for (int i = 0; i < rd->ncols; i++) {
     ResTarget* t = (ResTarget*)targetList->elements[i].ptr;
-    // we don't care about the data type or length here 
+    // we don't care about the data type, length, or nullability here
-    construct_column_desc(&rd->cols[i], t->name, DT_UNKNOWN, i, 0);
+    construct_column_desc(&rd->cols[i], t->name, DT_UNKNOWN, i, 0, true);
   }
 
   return rd;
 }
```

And here, we make sure to pass in a value to the new `isNotNull` parameter.

## RecordSet

Back to our top-level code, the next line we dive into is:

```c
    RecordSet* rs = new_recordset();
```

We have similar changes to the `RecordSet` regime as we had to `RecordDescriptor`. The changes are primarily focused on the `RecordSetRow` struct. Similar to how it keeps an array of column values, we also need to keep a `bool` array to identify if the column is null or not. This will make it easier for our output code to identify when a column's value is `NULL` or not.

`src/include/resultset/recordset.h`

```diff
 typedef struct RecordSetRow {
   Datum* values;
+  bool* isnull;
 } RecordSetRow;
```

`src/resultset/recordset.c`

```diff
 RecordSetRow* new_recordset_row(int ncols) {
   RecordSetRow* row = malloc(sizeof(RecordSetRow));
   row->values = malloc(ncols * sizeof(Datum));
+  row->isnull = malloc(ncols * sizeof(bool));
   return row;
 }
 
 void free_recordset_row(RecordSetRow* row, RecordDescriptor* rd) {
   if (row->values != NULL) {
     free_recordset_row_columns(row, rd);
     free(row->values);
+    free(row->isnull);
   }
   free(row);
 }
```

These basic changes simply allocate and free the memory reserved for the new `bool` array in a `RecordSetRow`.

## defill_record

Continuing along our top-level code block, the next line we dive into:

```c
    tableam_fullscan(bp, td, rs->rows);
```

This function makes a call to `defill_record`. Since we made several changes to `fill_record`, it only makes sense we have to refactor its opposite.

`src/access/tableam.c`

```diff
 void tableam_fullscan(BufPool* bp, TableDesc* td, LinkedList* rows) {
   BufPoolSlot* slot = bufpool_read_page(bp, 1);
 
   while (slot != NULL) {
     PageHeader* pgHdr = (PageHeader*)slot->pg;
     int numRecords = pgHdr->numRecords;
 
     for (int i = 0; i < numRecords; i++) {
       RecordSetRow* row = new_recordset_row(td->rd->ncols);
       int slotPointerOffset = conf->pageSize - (sizeof(SlotPointer) * (i + 1));
       SlotPointer* sp = (SlotPointer*)(slot->pg + slotPointerOffset);
-      defill_record(td->rd, slot->pg + sp->offset, row->values);
+      defill_record(td->rd, slot->pg + sp->offset, row->values, row->isnull);
       linkedlist_append(rows, row);
     }
 
     slot = bufpool_read_page(bp, pgHdr->nextPageId);
   }
 }
```

We simply need to pass it a reference to `ResultSetRow`'s array of nulls. The magic happens below..

`src/include/storage/record.h`

```diff
-void defill_record(RecordDescriptor* rd, Record r, Datum* values);
+void defill_record(RecordDescriptor* rd, Record r, Datum* values, bool* isnull);
```

`src/storage/record.c`

```diff
-void defill_record(RecordDescriptor* rd, Record r, Datum* values) {
+void defill_record(RecordDescriptor* rd, Record r, Datum* values, bool* isnull) {
   int offset = sizeof(RecordHeader);
+  uint16_t nullOffset = ((RecordHeader*)r)->nullOffset;
+  uint8_t* nullBitmap = r + nullOffset;
 
   Column* col;
   for (int i = 0; i < rd->ncols; i++) {
+    // we've passed the fixed-length, so we skip over the null bitmap
+    if (i == rd->nfixed) offset += compute_null_bitmap_length(rd);
 
     if (i < rd->nfixed) {
       col = get_nth_col(rd, true, i);
     } else {
       col = get_nth_col(rd, false, i - rd->nfixed);
     }
 
-    values[col->colnum] = record_get_col_value(col, r, &offset);
+    if (col_isnull(i, nullBitmap)) {
+      values[col->colnum] = (Datum)NULL;
+      isnull[col->colnum] = true;
+    } else {
+      values[col->colnum] = record_get_col_value(col, r, &offset);
+      isnull[col->colnum] = false;
     }
   }
 }
```

Because we're now dealing with `Null` columns, we need to find the null bitmap. Fortunately the `RecordHeader` stores a pointer to the location of the null bitmap, which makes it trivial to find the beginning of the null bitmap.

When we're reading data from a record, we read left to right. And remember fixed-length columns are physically stored on the left side of a record, while variable-length columns are stored on the right - separated by the null bitmap. So at the beginning of the loop, we need to check if we've read through all of the fixed-length columns. If so, we need to adjust the read pointer (`offset`) to skip past the null bitmap and start reading the variable-length columns.

THe last set of changes involves logic to set the `isnull` array appropriately. The check utilizes a new function that finds the corresponding bit in the null bitmap for the current column we're trying to read:

`src/include/storage/record.h`

```diff
+bool col_isnull(int colnum, uint8_t* nullBitmap);
```

`src/storage/record.c`

```diff
+bool col_isnull(int colnum, uint8_t* nullBitmap) {
+  return !(nullBitmap[colnum >> 3] & (1 << (colnum & 0x07)));
+}
```

This function takes two parameters: the column number in the table (counting from left to right as the data are physically stored), and a pointer to the null bitmap. 