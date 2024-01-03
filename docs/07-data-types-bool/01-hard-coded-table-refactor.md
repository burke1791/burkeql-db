# Hard-Coded Table Refactor

I don't know about you, but I'm starting to get real tired of refactoring our temporary code due to introducing a new data type. Rest assured, relief is on the horizon, but we're not there just yet.

Quick note: most of the text in this section will mostly be copy/pasted (with applicable changes) from the last section where we introduced the `Int`'s. That's because we're changing the code in all the same places, so you can skip to the code diff sections if you don't feel like reading the same walls of text again.

In this section, we're introducing the `Bool` data type. Its storage footprint will fill one byte and we'll be using the `uint8_t` C type in our code. The `Bool` is essentially a `TinyInt`, but will only have two valid values: true and false. Using a full byte to store something that only needs a single bit may seem inefficient... and it absolutely is. However, there's all sorts of extra logical overhead that comes with packing multiple `Bools` in a single byte that I don't want to deal with.

Some database engines, like Microsoft SQL Server, do indeed store boolean values in a single bit. In MSSQL's case, the data type is actually called a `Bit`. It still requires a full byte of storage space, but if a table contains more than one `Bit` column, the engine is able to stuff up to 8 of them in that single byte.

I want to keep our code simple, so we're going to use a full byte for each `Bool` column we come across.

## Table DDL

First thing's first, our new table definition. It's the same as last time, except we're adding a `Bool` column to the end:

```sql
Create Table person (
    person_id Int Not Null,
    name Char(20) Not Null,
    age TinyInt Not Null,
    daily_steps SmallInt Not Null,
    distance_from_home BigInt Not Null,
    is_alive Bool Not Null
);
```

And for fun let's calculate the byte requirement of a new record:

- 12-byte header
- 4-byte `person_id`
- 20-byte `name`
- 1-byte `age`
- 2-byte `daily_steps`
- 8-byte `distance_from_home`
- 1-byte `is_alive`
- 4-byte slot pointer
- **total**: 52 bytes

Since the page header consumes the first 20 bytes, we can just barely fit two records on our 128-byte data page.

## Refactoring

Let's start adding support for the new data type. Beginning witht he obvious: the `DataType` enum.

`src/include/storage/record.h`

```diff
 typedef enum DataType {
   DT_TINYINT,     /* 1-byte, unsigned */
   DT_SMALLINT,    /* 2-bytes, signed */
   DT_INT,         /* 4-bytes, signed */
   DT_BIGINT,      /* 8-bytes, signed */
+  DT_BOOL,        /* 1-byte, unsigned | similar to DT_TINYINT, but always evaluates to 1 or 0 */
   DT_CHAR,        /* Byte-size defined at table creation */
   DT_UNKNOWN
 } DataType;
```

Next, we can update all of our temporary functions in our `main.c` file:

`src/main.c`

```diff
-#define RECORD_LEN  47
+#define RECORD_LEN  48

-static void populate_datum_array(Datum* data, int32_t person_id, char* name, uint8_t age, int16_t dailySteps, int64_t  distanceFromHome) { 
+static void populate_datum_array(Datum* data, int32_t person_id, char* name, uint8_t age, int16_t dailySteps, int64_t  distanceFromHome, uint8_t isAlive) {
   data[0] = int32GetDatum(person_id);
   data[1] = charGetDatum(name);
   data[2] = uint8GetDatum(age);
   data[3] = int16GetDatum(dailySteps);
   data[4] = int64GetDatum(distanceFromHome);
+  data[5] = uint8GetDatum(isAlive);
 }
```

We need to update the `RECORD_LEN` macro to reflect the total footprint of our data records. Note it's 48 instead of 52 (from above) because this macro does not include the 4-byte slot pointer. We also piggyback on the existing `uint8` datum conversion function because that's how we're representing the `Bool` data type.

Next, we add parameters for each of the new columns in our table as well as calls to the datum conversion functions that we will need to write.

```diff
 static RecordDescriptor* construct_record_descriptor() {
-  RecordDescriptor* rd = malloc(sizeof(RecordDescriptor) + (5 * sizeof(Column)));
+  RecordDescriptor* rd = malloc(sizeof(RecordDescriptor) + (6 * sizeof(Column)));
-  rd->ncols = 5;
+  rd->ncols = 6;
 
   construct_column_desc(&rd->cols[0], "person_id", DT_INT, 0, 4);
   construct_column_desc(&rd->cols[1], "name", DT_CHAR, 1, 20);
   construct_column_desc(&rd->cols[2], "age", DT_TINYINT, 2, 1);
   construct_column_desc(&rd->cols[3], "daily_steps", DT_SMALLINT, 3, 2);
   construct_column_desc(&rd->cols[4], "distance_from_home", DT_BIGINT, 4, 8);
+  construct_column_desc(&rd->cols[5], "is_alive", DT_BOOL, 5, 1);
 
   return rd;
 }
```

We update our `RecordDescriptor` factory to create a `RecordDescriptor` with 6 columns now instead of 5, and add calls to `construct_column_desc` with the appropriate inputs for our new `DT_BOOL` column.

```diff
-static void serialize_data(RecordDescriptor* rd, Record r, int32_t person_id, char* name, uint8_t age, int16_t  dailySteps, int64_t distanceFromHome) {
+static void serialize_data(RecordDescriptor* rd, Record r, int32_t person_id, char* name, uint8_t age, int16_t  dailySteps, int64_t distanceFromHome, uint8_t isAlive) {
   Datum* data = malloc(rd->ncols * sizeof(Datum));
-  populate_datum_array(data, person_id, name, age, dailySteps, distanceFromHome);
+  populate_datum_array(data, person_id, name, age, dailySteps, distanceFromHome, isAlive);
   fill_record(rd, r + sizeof(RecordHeader), data);
   free(data);
 }
```

Our serializer function just needs to include the new column as a parameter, and pass it to the `populate_datum_array` function.

```diff
-static bool insert_record(BufPool* bp, int32_t person_id, char* name, uint8_t age, int16_t dailySteps, int64_t  distanceFromHome) {
+static bool insert_record(BufPool* bp, int32_t person_id, char* name, uint8_t age, int16_t dailySteps, int64_t  distanceFromHome, uint8_t isAlive) {
   BufPoolSlot* slot = bufpool_read_page(bp, 1);
   if (slot == NULL) slot = bufpool_new_page(bp);
   RecordDescriptor* rd = construct_record_descriptor();
   Record r = record_init(RECORD_LEN);
-  serialize_data(rd, r, person_id, name, age, dailySteps, distanceFromHome);
+  serialize_data(rd, r, person_id, name, age, dailySteps, distanceFromHome, isAlive);
   bool insertSuccessful = page_insert(slot->pg, r, RECORD_LEN);
 
   free_record_desc(rd);
   free(r);
   
   return insertSuccessful;
 }
```

Similar to the above, we just add the new parameter and pass it along.

```diff
 static bool analyze_selectstmt(SelectStmt* s) {
   for (int i = 0; i < s->targetList->length; i++) {
     ResTarget* r = (ResTarget*)s->targetList->elements[i].ptr;
     if (
       !(
         strcasecmp(r->name, "person_id") == 0 ||
         strcasecmp(r->name, "name") == 0 ||
         strcasecmp(r->name, "age") == 0 ||
         strcasecmp(r->name, "daily_steps") == 0 ||
-        strcasecmp(r->name, "distance_from_home") == 0
+        strcasecmp(r->name, "distance_from_home") == 0 ||
+        strcasecmp(r->name, "is_alive") == 0
       )
     ) {
       return false;
     }
   }
   return true;
 }
```

And the last of our temporary functions, the analyzer, just needs to check for the new column in the `Select` target list.

That's it for refactoring the temporary code. Next up, we'll refactor the parser to make sure it looks for the new column in its insert statement grammar.