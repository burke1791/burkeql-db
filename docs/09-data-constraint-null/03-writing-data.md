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
int bitmask: 10000000 (binary) only showing the first byte
```

Note: although `bitmask` is a 4-byte int, I'm only going to show the first byte throughout this example because that's the only part of it that's relevant.

Next, we go into the loop where we `fill_val` all of the fixed-length columns. Our table has two fixed-length columns, so we start with `person_id` and pass in the appropriate parameters. Relevant to this example, we supply `&bitP` - a pointer to the pointer that currently points to the byte preceeding the null bitmap. And `&bitmask` - a pointer to the `bitmask` variable.

### fill_val - person_id

Since we already know how this function writes data to our `Record`, I'm just going to focus on the new null bitmap operations. At the top, we're immediately confronted with this if/else statement:

```c
if (*bitmask != 0x80) {
  *bitmask <<= 1;
} else {
  *bit += 1;
  **bit = 0x0;
  *bitmask = 1;
}
```

If `bitmask` is not equal to `0x80` (`10000000`), then we left shift `bitmask` by one. If you remember in `fill_record`, we explicitly initialized `bitmask` to `0x80`, so we can proceed to the `else` block.

`*bit += 1;`

Here, we advance the bitmap pointer by one. Remember how we started out by pointing to the byte preceeding the null bitmap? This is where we course-correct by pointing to the actual beginning of the null bitmap.

`**bit = 0x0;`

Next, we set the entire byte of the null bitmap to 0s. It doesn't matter how large the null bitmap is, this instruction will only set a single byte to 0s.

`*bitmask = 1;`

Reset the bitmask to 1 (`00000001`).

Here's the current state of our variables:

```
uint8_t* bitP: pointing to the beginning of the null bitmap
int bitmask: 00000001 (binary)

person_id | bitmap   | last_name
0000 0000 | 00000000 | 0000 0000 0000 00
```

Why the convoluted mess with the if/else block and bitwise operations? Its purpose is to reset the bitmask back to 1 each time we hit a byte boundary. That situation won't come into play during this example because we only have four columns to worry about. Since each byte in the null bitmap can cover 8 columns, that logic is only important for tables with more than 8 columns.

Now, why do we need to reset the bitmask at byte boundaries? Because the bitmap pointer (`bitP`) always points to the byte we're currently working with. We have a bitwise OR operation coming up between the bitmap byte and the bitmask. In order for this to work properly, we need to make sure we're only using the first byte of the bitmask variable.

Final question: why do we need to advance the `bitP` pointer? Can't we just keep it pointing to the first byte in the null bitmap so that left-shifting the bitmask after 8 columns will still work? Short answer: no. What if we have a table with more than 32 columns? Left-shifting the bitmask after that would move it outside of its 4-byte range and the bitwise OR operation would no longer work.

Moving on, we have two more lines before we write data to `Record`:

```c
  // column is null, nothing more to do
  if (isNull) return;

  **bit |= *bitmask;
```

In this example `person_id` has a value, so we do not need to return early. Now, we bitwise OR the current null bitmap byte with the first byte of `bitmask`, then assign the result to the null bitmap byte. It looks like this:

```
00000000  <-- bitmap
00000001  <-- bitmask
--------
00000001  <-- result
```

Then we continue to write `person_id` to the `Record`. Now, our variables look like this:

```
uint8_t* bitP: pointing to the beginning of the null bitmap
int bitmask: 00000001 (binary)

person_id | bitmap   | last_name
0100 0000 | 00000001 | 0000 0000 0000 00
```

Remember, we're dealing with a little endian machine, so the first byte of `person_id` is what's populated. And the first bit in the null bitmap signifies that a value is present for the first column. `1` means the column has a value, `0` means the column is `Null`.

### fill_val - age

Now let's see how we handle a Null value. Current state:

```
uint8_t* bitP: pointing to the beginning of the null bitmap
int bitmask: 00000001 (binary)

person_id | bitmap   | last_name
0100 0000 | 00000001 | 0000 0000 0000 00
```

And our if/else block of code:

```c
if (*bitmask != 0x80) {
  *bitmask <<= 1;
} else {
  *bit += 1;
  **bit = 0x0;
  *bitmask = 1;
}
```

Since `bitmask` is not equal to `0x80` (i.e. we're not at a byte boundary), we get to perform the left-shift operation this time.

```
uint8_t* bitP: pointing to the beginning of the null bitmap
int bitmask: 00000010 (binary)  <------ THIS CHANGED

person_id | bitmap   | last_name
0100 0000 | 00000001 | 0000 0000 0000 00
```

The value of `bitmask` went from 1 to 2 as a result of the left-shift.

Next, we have:

```c
  // column is null, nothing more to do
  if (isNull) return;

  **bit |= *bitmask;
```

Since `age` is Null, we exit the function and proceed to the variable-length columns.

### fill_val - first_name

Current state:

```
uint8_t* bitP: pointing to the beginning of the null bitmap
int bitmask: 00000010 (binary)

person_id | bitmap   | last_name
0100 0000 | 00000001 | 0000 0000 0000 00
```

Since `first_name` is also Null, we perform the exact same actions we did for `age`: left-shift `bitmask` then `return`.

### fill_val - last_name

Current state:

```
uint8_t* bitP: pointing to the beginning of the null bitmap
int bitmask: 00000100 (binary)

person_id | bitmap   | last_name
0100 0000 | 00000001 | 0000 0000 0000 00
```

First up, we have our if/else block:

```c
if (*bitmask != 0x80) {
  *bitmask <<= 1;
} else {
  *bit += 1;
  **bit = 0x0;
  *bitmask = 1;
}
```

Our current state satisfies the condition check, so we left-shift `bitmask`.

New state:

```
uint8_t* bitP: pointing to the beginning of the null bitmap
int bitmask: 00001000 (binary)

person_id | bitmap   | last_name
0100 0000 | 00000001 | 0000 0000 0000 00
```

```c
  // column is null, nothing more to do
  if (isNull) return;

  **bit |= *bitmask;
```

And because `last_name` has a value, we perform the bitwise OR then proceed to writing data to `Record`. The bitwise OR operation looks like this:

```
00000001  <-- bitmap
00001000  <-- bitmask
--------
00001001  <-- result
```

This results in the final state of our record looking like:

```
uint8_t* bitP: pointing to the beginning of the null bitmap
int bitmask: 00001000 (binary)

person_id | bitmap   | last_name
0100 0000 | 00001001 | 0700 4275 726b 65 
                         ^   B u  r k  e
                         |
                         |----  2-byte variable-length header 
```

To recap, we have a null bitmap that looks like `00001001`. Reading right to left, it tells us the first and fourth columns are Not Null, while the 2nd and 3rd columns are Null.

## Finishing Up

The last two changes I need to highlight are all the way back in the `serialize_data` function. We need to free the memory for used by our two new `fixedNull` and `varlenNull` arrays.

Now we're ready to run some inserts and inspect the data file. Remember to delete your existing data file before running the program with our new code.

```shell
$ rm -f db_files/main.dbd
$ make clean && make && ./burkeql
======   BurkeQL Config   ======
= DATA_FILE: /home/burke/source_control/burkeql-db/db_files/main.dbd
= PAGE_SIZE: 128
bql > insert 1 Null 'Burke' 33;
======  Node  ======
=  Type: Insert
=  person_id           1
=  first_name          NULL
=  last_name           Burke
=  age                 33
Bytes read: 0
bql >
```

I run `insert 1 Null 'Burke' 33;` to insert some data with the `first_name` column being Null. Let's take a look at the data file.

```shell
$ xxd db_files/main.dbd
00000000: 0100 0000 0000 0000 0000 0000 0000 0100  ................
00000010: 4c00 4c00 0000 0000 0000 0000 0000 1400  L.L.............
00000020: 0100 0000 2100 0000 0b07 0042 7572 6b65  ....!......Burke
00000030: 0000 0000 0000 0000 0000 0000 0000 0000  ................
00000040: 0000 0000 0000 0000 0000 0000 0000 0000  ................
00000050: 0000 0000 0000 0000 0000 0000 0000 0000  ................
00000060: 0000 0000 0000 0000 0000 0000 0000 0000  ................
00000070: 0000 0000 0000 0000 0000 0000 1400 1c00  ................
```

Not particularly interesting since we can't see the bits in the null bitmap. Let's look at the 1s and 0s by using the `-b` flag:

```shell
$ xxd -b db_files/main.dbd
00000000: 00000001 00000000 00000000 00000000 00000000 00000000  ......
00000006: 00000000 00000000 00000000 00000000 00000000 00000000  ......
0000000c: 00000000 00000000 00000001 00000000 01001100 00000000  ....L.
00000012: 01001100 00000000 00000000 00000000 00000000 00000000  L.....
00000018: 00000000 00000000 00000000 00000000 00000000 00000000  ......
0000001e: 00010100 00000000 00000001 00000000 00000000 00000000  ......
00000024: 00100001 00000000 00000000 00000000 00001011 00000111  !.....  <-- Null bitmap is second to last byte
0000002a: 00000000 01000010 01110101 01110010 01101011 01100101  .Burke
00000030: 00000000 00000000 00000000 00000000 00000000 00000000  ......
00000036: 00000000 00000000 00000000 00000000 00000000 00000000  ......
0000003c: 00000000 00000000 00000000 00000000 00000000 00000000  ......
00000042: 00000000 00000000 00000000 00000000 00000000 00000000  ......
00000048: 00000000 00000000 00000000 00000000 00000000 00000000  ......
0000004e: 00000000 00000000 00000000 00000000 00000000 00000000  ......
00000054: 00000000 00000000 00000000 00000000 00000000 00000000  ......
0000005a: 00000000 00000000 00000000 00000000 00000000 00000000  ......
00000060: 00000000 00000000 00000000 00000000 00000000 00000000  ......
00000066: 00000000 00000000 00000000 00000000 00000000 00000000  ......
0000006c: 00000000 00000000 00000000 00000000 00000000 00000000  ......
00000072: 00000000 00000000 00000000 00000000 00000000 00000000  ......
00000078: 00000000 00000000 00000000 00000000 00010100 00000000  ......
0000007e: 00011100 00000000                                      ..
```

I'm feeling lazy and don't want to draw colorful boxes around a screenshot this time, so I'll describe it. The interesting piece here is the null bitmap. It's on the line that starts with `00000024:` and is the second to last byte on the row. Its bitfields are `00001011`. Reading right to left, it tells us the 1st, 3rd, and 4th columns are Not Null, and the 2nd column is Null.

And that's all there is to incorporating the null bitmap in our "write data" operations. In the next section we'll go through the "read data" code.