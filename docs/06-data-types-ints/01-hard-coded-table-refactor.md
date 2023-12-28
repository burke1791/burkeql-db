# Hard-Coded Table Refactor

In this section we're going to implement the storage component of the remaining "Int" data types: `TinyInt`, `SmallInt`, and `BigInt`. Since we already have the plumbing in place for the 4-byte `Int`, this won't be a very difficult addition to implement. However, since we're still working with a hard-coded table definition, we need to (annoyingly) refactor the code associated with that.

I know what you're thinking.. when can we ditch the hard-coded table definition? Somewhat soon. We need to introduce the system tables and build a legitimate analyzer first. And we'll be diving into those exact pieces right after we finish expanding our supported data types.

As for the `Ints`, we'll need to work with a different hard-coded table - one that has all four of the different "Int" types:

```sql
Create Table person (
    person_id Int Not Null,
    name Char(20) Not Null,
    age TinyInt Not Null,
    daily_steps SmallInt Not Null,
    distance_from_home BigInt Not Null
);
```

Using the same table we've been working with, I added three new columns, `age TinyInt`, `daily_steps SmallInt`, and `distance_from_home BigInt`.. because maybe our person is an astronaut that's billions of miles from Earth. I was reaching here.

The three new data types have different storage footprints compared to the 4-byte `Int`.

| Data Type | Size | Min Value | Max Value |
|-----------|------|-----------|-----------|
| TinyInt | 1-byte | 0 | 255 |
| SmallInt | 2-bytes | -32,768 | 32,767 |
| Int | 4-bytes | -2,147,483,648 | 2,147,483,647 |
| BigInt | 8-bytes | -9,223,372,036,854,775,808 | 9,223,372,036,854,775,807 |

Yes, the `BigInt` is absurdly large. Fun fact, a `UUID` is just a large number and is twice the size (16-bytes) of a `BigInt`. So when you hear that `UUID`'s are pretty much guaranteed to be unique.. they're GUARANTEED to be unique.

Now that we know the sizes of our new data types, we can calculate the number of bytes each row in our table will need:

- 12-byte header
- 4-byte `person_id`
- 20-byte `name`
- 1-byte `age`
- 2-byte `daily_steps`
- 8-byte `distance_from_home`
- 4-byte slot pointer
- **total**: 51 bytes

Since the page header consumes the first 20 bytes, we can just barely fit two records on our 128-byte page.

## Refactoring

Let's start adding support for these new data types. Beginning with the obvious: the `DataType` enum.

`src/include/storage/record.h`

```diff
 typedef enum DataType {
+  DT_TINYINT,     /* 1-byte, unsigned */
+  DT_SMALLINT,    /* 2-bytes, signed */
   DT_INT,         /* 4-bytes, signed */
+  DT_BIGINT,      /* 8-bytes, signed */
   DT_CHAR,        /* Byte-size defined at table creation */
   DT_UNKNOWN
 } DataType;
```

Next, we can update all of our temporary functions in our `main.c` file:

`src/main.c`

```diff
-#define RECORD_LEN  36  // 12-byte header + 4-byte Int + 20-byte Char(20)
+#define RECORD_LEN  47

-static void populate_datum_array(Datum* data, int32_t person_id, char* name) {
+static void populate_datum_array(Datum* data, int32_t person_id, char* name, uint8_t age,  int16_t dailySteps, int64_t distanceFromHome) {
   data[0] = int32GetDatum(person_id);
   data[1] = charGetDatum(name);
+  data[2] = uint8GetDatum(age);
+  data[3] = int16GetDatum(dailySteps);
+  data[4] = int64GetDatum(distanceFromHome);
 }
```

We need to update the `RECORD_LEN` macro to reflect the total footprint of our data records. Note it's 47 instead of 51 (from above) because this macro does not include the 4-byte slot pointer.

Next, we add parameters for each of the new columns in our table as well as calls to the datum conversion functions that we will need to write.

```diff
 static RecordDescriptor* construct_record_descriptor() {
-  RecordDescriptor* rd = malloc(sizeof(RecordDescriptor) + (2 * sizeof(Column)));
+  RecordDescriptor* rd = malloc(sizeof(RecordDescriptor) + (5 * sizeof(Column)));
-  rd->ncols = 2;
+  rd->ncols = 5;
 
   construct_column_desc(&rd->cols[0], "person_id", DT_INT, 0, 4);
   construct_column_desc(&rd->cols[1], "name", DT_CHAR, 1, 20);
+  construct_column_desc(&rd->cols[2], "age", DT_TINYINT, 2, 1);
+  construct_column_desc(&rd->cols[3], "daily_steps", DT_SMALLINT, 3, 2);
+  construct_column_desc(&rd->cols[4], "distance_from_home", DT_BIGINT, 4, 8);
 
   return rd;
 }
```

We update our `RecordDescriptor` factory to create a `RecordDescriptor` with 5 columns now instead of two, and add calls to `construct_column_desc` with the appropriate inputs for our new columns.

```diff
-static void serialize_data(RecordDescriptor* rd, Record r, int32_t person_id, char* name) {
+static void serialize_data(RecordDescriptor* rd, Record r, int32_t person_id, char* name,  uint8_t age, int16_t dailySteps, int64_t distanceFromHome) {
   Datum* data = malloc(rd->ncols * sizeof(Datum));
-  populate_datum_array(data, person_id, name);
+  populate_datum_array(data, person_id, name, age, dailySteps, distanceFromHome);
   fill_record(rd, r + sizeof(RecordHeader), data);
   free(data);
 }
```

Our serializer function just needs to include the new columns as parameters, and pass those values to the `populate_datum_array` function.

```diff
-static bool insert_record(BufPool* bp, int32_t person_id, char* name) {
+static bool insert_record(BufPool* bp, int32_t person_id, char* name, uint8_t age, int16_t  dailySteps, int64_t distanceFromHome) {
   BufPoolSlot* slot = bufpool_read_page(bp, 1);
   if (slot == NULL) slot = bufpool_new_page(bp);
   RecordDescriptor* rd = construct_record_descriptor();
   Record r = record_init(RECORD_LEN);
-  serialize_data(rd, r, person_id, name);
+  serialize_data(rd, r, person_id, name, age, dailySteps, distanceFromHome);
   bool insertSuccessful = page_insert(slot->pg, r, RECORD_LEN);
 
   free_record_desc(rd);
   free(r);
   
   return insertSuccessful;
 }
```

Similar to the above, we just add the new parameters and pass them along.

```diff
 static bool analyze_selectstmt(SelectStmt* s) {
   for (int i = 0; i < s->targetList->length; i++) {
     ResTarget* r = (ResTarget*)s->targetList->elements[i].ptr;
-    if (!(strcasecmp(r->name, "person_id") == 0 || strcasecmp(r->name, "name") == 0)) return  false;
+    if (
+      !(
+        strcasecmp(r->name, "person_id") == 0 ||
+        strcasecmp(r->name, "name") == 0 ||
+        strcasecmp(r->name, "age") == 0 ||
+        strcasecmp(r->name, "daily_steps") == 0 ||
+        strcasecmp(r->name, "distance_from_home") == 0
+      )
+    ) {
+      return false;
+    }
   }
   return true;
 }
```

And the last of our temporary functions, the analyzer, just needs to check for the new columns in the `Select` target list.

That's it for refactoring the temporary code. Next up, we'll refactor the parser to make sure it looks for the new columns in its insert statement grammar.