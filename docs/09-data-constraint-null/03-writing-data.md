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

## Record Data

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

As for the refactored `populate_datum_array` function, it is now going to populate the two null arrays alongside the values arrays. And the function is changing so much, we're just going to scrap it and write a new one from scratch.

`src/main.c`

```c
static void populate_datum_array(Datum* fixed, Datum* varlen, bool* fixedNull, bool* varlenNull, ParseList* values) {
  Literal* personId = (Literal*)values->elements[0].ptr;
  Literal* firstName = (Literal*)values->elements[1].ptr;
  Literal* lastName = (Literal*)values->elements[2].ptr;
  Literal* age = (Literal*)values->elements[3].ptr;

  fixed[0] = int32GetDatum(personId->intVal);
  fixedNull[0] = false;

  if (age->isNull) {
    fixed[1] = (Datum)NULL;
    fixedNull[1] = true;
  } else {
    fixed[1] = int32GetDatum(age->intVal);
    fixedNull[1] = false;
  }
  
  if (firstName->isNull) {
    varlen[0] = (Datum)NULL;
    varlenNull[0] = true;
  } else {
    varlen[0] = charGetDatum(firstName->str);
    varlenNull[0] = false;
  }
  
  varlen[1] = charGetDatum(lastName->str);
  varlenNull[1] = false;
}
```

First we parse out the `Literal`'s for each of our four hard-coded columns. Then we start filling in the value and null arrays. Since `person_id` is `Not Null`, we don't need a null check and can assign the values directly. `age`, on the other hand, can be `Null`, so we perform a null check before populating the arrays. Notice for the null case we cast `NULL` to a `Datum`. Why? Because our array expects a `Datum` type and the compiler will yell at us otherwise.

We follow similar logic for the variable-length columns. `first_name` is `Null`able, so we need a null check, whereas `last_name` is not.

Jumping back up to the `serialize_data` function, the next change we come across is `fill_record`. We have three additional parameters to pass in to the new version of the function: both null arrays and a pointer to the null bitmap. Let's update the header file first, then get to the code.

`src/include/storage.record.h`

```diff
-void fill_record(RecordDescriptor* rd, Record r, Datum* fixed, Datum* varlen);
+void fill_record(RecordDescriptor* rd, Record r, Datum* fixed, Datum* varlen, bool* fixedNull, bool* varlenNull, uint8_t* bitmap);
```

`src/storage/record.c`

```diff
-void fill_record(RecordDescriptor* rd, Record r, Datum* fixed, Datum* varlen) {
+void fill_record(RecordDescriptor* rd, Record r, Datum* fixed, Datum* varlen, bool* fixedNull, bool* varlenNull, uint8_t* nullBitmap) {
+  uint8_t* bitP = &nullBitmap[-1];
+  int bitmask = 0x80;
 
   // fill fixed-length columns
   for (int i = 0; i < rd->nfixed; i++) {
     Column* col = get_nth_col(rd, true, i);
 
-    fill_val(col, &r, fixed[i]);
+    fill_val(
+      col,
+      &bitP,
+      &bitmask,
+      &r,
+      fixed[i],
+      fixedNull[i]
+    );
   }
 
+  // jump past the null bitmap
+  r += compute_null_bitmap_length(rd);
 
   // fill varlen columns
   for (int i = 0; i < (rd->ncols - rd->nfixed); i++) {
     Column* col = get_nth_col(rd, false, i);
-    fill_val(col, &r, varlen[i]);
+    fill_val(
+      col,
+      &bitP,
+      &bitmask,
+      &r,
+      varlen[i],
+      varlenNull[i]
+    );
   }
 }
```

Since we're introducing the null bitmap, we'll need to start writing some bitwise operations. That's where these first two lines of the function come in. The actual bitwise operations will happen in `fill_val`, but we need to pass in a pointer to the null bitmap that it can operate on, as well as a bitmask to use for populating bits in the null bitmap.

Why do we assign `bitP` to the -1 index? Is that legal? Although it looks like an invalid address, it technically is because the null bitmap is inside the `Record` chunk of memory. The -1 index just means it's pointing to the byte immediately preceeding the null bitmap. We'll go over the "why" below when we walk through an example.

`src/storage/record.c`

```diff
-static void fill_val(Column* col, char** dataP, Datum datum) {
+static void fill_val(Column* col, uint8_t** bit, int* bitmask, char** dataP, Datum datum, bool  isNull) {
   int16_t dataLen;
   char* data = *dataP;
 
+  if (*bitmask != 0x80) {
+    *bitmask <<= 1;
+  } else {
+    *bit += 1;
+    **bit = 0x0;
+    *bitmask = 1;
+  }
+
+  // column is null, nothing more to do
+  if (isNull) return;
+
+  **bit |= *bitmask;
 
   switch (col->dataType) {
     case DT_BOOL:     // Bools and TinyInts are the same C-type
     case DT_TINYINT:
 
*** omitted for brevity ***
 
 }
```

Working with the actual data remains the same as before. The only difference is we added some logic to populate the bitfields in the null bitmap and short-circuit the function if the value to insert is `Null`. So how and why does this work?

## Step-by-Step Example

Let's walk through an example line by line. Pretend we're the computer and we're trying to insert the following record:

| person_id | first_name | last_name | age |
|-----------|------------|-----------|-----|
| 1 | Null | Burke | Null |

Ignoring the row header, how many bytes do we need to store this record? In physical order of the data:

- 4-byte `person_id`
- 0-byte `age` (Null)
- 1-byte null bitmap
- 0-byte `first_name` (Null)
- 7-byte `last_name` (2-byte overhead plus 5-bytes for data)
- **Total**: 12 bytes

We start with a blank chunk of memory that represents the `Record` we're going to fill. Again, I'm ignoring the header portion in this example. Here's what we're working with:

```
person_id | bitmap   | last_name
0000 0000 | 00000000 | 0000 0000 0000 00
```

**IMPORTANT**: I'm representing `person_id` and `last_name` in hex like the `xxd` command shows, but I'm showing all 8 individual bits for the null bitmap portion.

### fill_record

Starting at the top, we initialize two variables:

```
uint8_t* bitP: points to the last byte of the person_id data
int bitmask: 
```