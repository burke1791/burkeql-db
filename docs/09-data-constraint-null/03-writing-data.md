# Writing Data

Now that our parser can handle the new way we parse insert statements, we can write the code that actually handles `Null` data and how it gets persisted to disk.

## main.c File

Like we've done before, we're going to pretend we're an insert statement and walk through the code path we'd follow, updating things accordingly. First up is our `main` function:

`src/main.c`

```diff
         break;
       case T_InsertStmt: {
-        int32_t person_id = ((InsertStmt*)n)->personId;
-        char* firstName = ((InsertStmt*)n)->firstName;
-        char* lastName = ((InsertStmt*)n)->lastName;
-        int32_t age = ((InsertStmt*)n)->age;
+        InsertStmt* i = (InsertStmt*)n;
-        if (!insert_record(bp, person_id, firstName, lastName, age)) {
+        if (!insert_record(bp, i->values)) {
           printf("Unable to insert record\n");
         }
         break;
       }
       case T_SelectStmt:
         if (!analyze_node(n)) {
```

First, we get rid of references to the hard-coded fields in the old `InsertStmt` struct. Then in our call to `insert_record`, we just pass in the `values`.

Which means we need to go refactor the `insert_record` function:

`src/main.c`

```diff
-static bool insert_record(BufPool* bp, int32_t person_id, char* firstName, char* lastName, int32_t age) {
+static bool insert_record(BufPool* bp, ParseList* values) {
   BufPoolSlot* slot = bufpool_read_page(bp, 1);
   if (slot == NULL) slot = bufpool_new_page(bp);
   RecordDescriptor* rd = construct_record_descriptor();
 
-  int recordLength = compute_record_length(rd, person_id, firstName, lastName, age);
+  int recordLength = compute_record_length(rd, values);
   Record r = record_init(recordLength);
 
-  serialize_data(rd, r, person_id, firstName, lastName, age);
+  serialize_data(rd, r, values);
   bool insertSuccessful = page_insert(slot->pg, r, recordLength);
 
   free_record_desc(rd);
   free(r);
   
   return insertSuccessful;
 }
```

The logic is exactly the same, but we need to replace the hard-coded input parameters for a single `ParseList* values` param. Then we replace any function calls that consumed those hard-coded parameters with a call that only passes in the `values` list. Now we need to go refactor `compute_record_length` and `serialize_data`.

`src/main.c`

```c
static int compute_record_length(RecordDescriptor* rd, ParseList* values) {
  int len = 12; // start with the 12-byte header
  len += compute_null_bitmap_length(rd);

  len += 4; // person_id

  Literal* firstName = (Literal*)values->elements[1].ptr;
  Literal* lastName = (Literal*)values->elements[2].ptr;
  Literal* age = (Literal*)values->elements[3].ptr;

  /* for the varlen columns, we default to their max length if the values
     we're trying to insert would overflow them (they'll get truncated later) */
  if (firstName->isNull) {
    len += 0; // Nulls do not consume any space
  } else if (strlen(firstName->str) > rd->cols[1].len) {
    len += (rd->cols[1].len + 2);
  } else {
    len += (strlen(firstName->str) + 2);
  }

  // We don't need a null check because this column is constrained to be Not Null
  if (strlen(lastName->str) > rd->cols[2].len) {
    len += (rd->cols[2].len + 2);
  } else {
    len += (strlen(lastName->str) + 2);
  }

  if (age->isNull) {
    len += 0; // Nulls do not consume any space
  } else {
    len += 4; // age
  }

  return len;
}
```

So much of this function changed, that I'm just going to call it a full rewrite. This is definitely a temporary function because it only works due to the assumptions we use. Starting at the top, we have 12 bytes for the record header - that's fine, no way around it. Next, we add the length of the null bitmap. And for that we create a new function to calculate its size - covered below.

Next, we add 4 bytes for the `person_id` column. Since we know it cannot be `Null` (assumption), we know it ALWAYS consumes 4 bytes.

Next, we parse out the `Literal`'s of the remaining three columns. For `firstName`, we check if the value passed in by the user is `Null` or not. If so, we don't add anything to the record's length because `Null`'s are absent in the physical record. If it's `Not Null`, then we perform the same logic as before: if the value's length exceeds the column's max length, we add max length + 2 to the record's length.

For the `lastName` field, we can skip the null check because we know it cannot be `Null` (assumption), and perform the same logic we do for the `firstName` field.

Lastly, we have a null check on the `age` column. If `Null`, the length is 0. If `Not Null`, the length is 4.

How do we determine the size of the null bitmap? Simple, each column corresponds to a single bit in the null bitmap. For table with 1 to 8 columns, the null bitmap consumes 1 byte of space. If the table has 9 to 16 columns, the null bitmap consumes 2 bytes. And so on.

`src/include/storage/record.h`

```diff
+int compute_null_bitmap_length(RecordDescriptor* rd);
```

`src/storage/record.c`

```diff
+/**
+ * returns the number of bytes consumed by the null bitmap
+ * 
+ * Every 8 columns requires an additional byte for the null bitmap
+*/
+int compute_null_bitmap_length(RecordDescriptor* rd) {
+  if (!rd->hasNullableColumns) return 0;
+
+  return (rd->ncols) / 8 + 1;
+}
 
 int compute_record_fixed_length(RecordDescriptor* rd, bool* fixedNull) {
```

This is actually going to be a permanent function, so we get to write it in the `record.c` file. Basically, the null bitmap consumes 1 byte for every 8 columns in the table.

Okay, back to the `insert_record` function. The next refactoring target on our path is the `serialize_data` function:

`src/main.c`

```diff
-static void serialize_data(RecordDescriptor* rd, Record r, int32_t person_id, char* firstName, char* lastName, int32_t age) { 
 static void serialize_data(RecordDescriptor* rd, Record r, ParseList* values) {
   Datum* fixed = malloc(rd->nfixed * sizeof(Datum));
   Datum* varlen = malloc((rd->ncols - rd->nfixed) * sizeof(Datum));
+  bool* fixedNull = malloc(rd->nfixed * sizeof(bool));
+  bool* varlenNull = malloc((rd->ncols - rd->nfixed) * sizeof(bool));
 
-  populate_datum_array(fixed, varlen, person_id, firstName, lastName, age);
   populate_datum_array(fixed, varlen, fixedNull, varlenNull, values);
 
+  int nullOffset = sizeof(RecordHeader) + compute_record_fixed_length(rd, fixedNull);
+  ((RecordHeader*)r)->nullOffset = nullOffset;
 
+  uint8_t* nullBitmap = r + nullOffset;
-  fill_record(rd, r + sizeof(RecordHeader), fixed, varlen);
+  fill_record(rd, r + sizeof(RecordHeader), fixed, varlen, fixedNull, varlenNull, nullBitmap);
 
   free(fixed);
   free(varlen);
+  free(fixedNull);
+  free(varlenNull);
 }
```

First, we do the expected replacement of the hard-coded column parameters for the `ParseList* values` param. Then we need to create a pair of `bool` arrays to match the `Datum` arrays for the fixed and variable-length fields. The `bool` arrays are used to indicate whether or not the corresponding value is `Null` or not.

Next, we make a call to the refactored `populate_datum_array` function. The refactored version (covered below) will populate values for our two `Datum` arrays and our two `bool` arrays based on the user-supplied `values` data.

Following that, we have our very first usage of a `RecordHeader` field. We need to set the `nullOffset` field to represent the position of the null bitmap in the `Record` data. Remember, the null bitmap lives between the fixed-length columns and the variable-length columns. So its location can be calculated by `(record header size) + (fixed-length size)`. And in order to compute the fixed-length size, we need a new (permanent) helper function.

Next, we have a refactored version of the `fill_record` function. We need to pass it the two new `bool` arrays, and a pointer to the null bitmap.

Finally, we need to free the memory consumed by the two new `bool` arrays.

