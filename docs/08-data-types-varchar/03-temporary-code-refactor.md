# Temporary Code Refactor

Ugh, I know. You hate it. I hate it. But we're getting closer to ditching it for good. If you look at the project roadmap, the "System Catalog" sections are when we'll drop the temp code.

Anyways, let's not drag this out and just get it over with.

All diffs are in the `main.c` file.

```diff
-#define RECORD_LEN  48
 #define BUFPOOL_SLOTS  1
```

We can get rid of the `RECORD_LEN` macro because we no longer need to use it (you'll see why below).

```c
static void populate_datum_array(Datum* fixed, Datum* varlen, int32_t person_id, char* firstName, char* lastName, int32_t age) {
  fixed[0] = int32GetDatum(person_id);
  fixed[1] = int32GetDatum(age);

  varlen[0] = charGetDatum(firstName);
  varlen[1] = charGetDatum(lastName);
}
```

This function is a full rewrite. Since we're switching from a single `Datum` array to two separate arrays for the fixed and variable length columns, we can just scrap the only function and use this new version.

```c
static RecordDescriptor* construct_record_descriptor() {
  RecordDescriptor* rd = malloc(sizeof(RecordDescriptor) + (4 * sizeof(Column)));
  rd->ncols = 4;
  rd->nfixed = 2;

  construct_column_desc(&rd->cols[0], "person_id", DT_INT, 0, 4);
  construct_column_desc(&rd->cols[1], "first_name", DT_VARCHAR, 1, 20);
  construct_column_desc(&rd->cols[2], "last_name", DT_VARCHAR, 2, 20);
  construct_column_desc(&rd->cols[3], "age", DT_INT, 3, 4);

  return rd;
}
```

Same thing goes for the `RecordDescriptor` factory. So much of the function guts changed, providing a diff would be too noisy.

Notice, we're now setting `rd->nfixed = 2;` to support the two column buckets. Also of note, this is where we set the colloquial ordering of the table's columns. You can see the `age` column is index 3 - the final column in the table. The "fill" code we wrote in the previous section ensures it will end up before the varchar columns despite being defined as after them.

```c
static void serialize_data(RecordDescriptor* rd, Record r, int32_t person_id, char* firstName, char* lastName, int32_t age) {
  Datum* fixed = malloc(rd->nfixed * sizeof(Datum));
  Datum* varlen = malloc((rd->ncols - rd->nfixed) * sizeof(Datum));

  populate_datum_array(fixed, varlen, person_id, firstName, lastName, age);
  fill_record(rd, r + sizeof(RecordHeader), fixed, varlen);
  free(fixed);
  free(varlen);
}
```

Another function whose guts almost entirely changed due to the separate fixed/varlen `Datum` arrays. The logic is the same, however, so I don't really need to explain what's going on.

```c
static int compute_record_length(RecordDescriptor* rd, int32_t person_id, char* firstName, char* lastName, int32_t age) {
  int len = 12; // start with the 12-byte header
  len += 4; // person_id

  /* for the varlen columns, we default to their max length if the values
     we're trying to insert would overflow them (they'll get truncated later) */
  if (strlen(firstName) > rd->cols[1].len) {
    len += (rd->cols[1].len + 2);
  } else {
    len += (strlen(firstName) + 2);
  }

  if (strlen(lastName) > rd->cols[2].len) {
    len += (rd->cols[2].len + 2);
  } else {
    len += (strlen(lastName) + 2);
  }

  len += 4; // age

  return len;
}
```

This is a brand new function and is the reason we no longer need the `RECORD_LEN` macro. Since `Varchar` columns can differ in length on a row-by-row basis, we need a way to calculate the byte-length of a row on the fly.

This is very much a hard-coded implementation. For the fixed-length columns (and the header) we can just add their static lengths to the total length. But for the `Varchar` columns, we need to compute the `strlen` of the incoming values. If the `strlen` exceeds the column's max length (defined in the `Column*` struct in the `RecordDescriptor`), then we set its length to the max length.

```diff
-static bool insert_record(BufPool* bp, int32_t person_id, char* name, uint8_t age, int16_t dailySteps, int64_t distanceFromHome, uint8_t isAlive) {
+static bool insert_record(BufPool* bp, int32_t person_id, char* firstName, char* lastName,  int32_t age) {
   BufPoolSlot* slot = bufpool_read_page(bp, 1);
   if (slot == NULL) slot = bufpool_new_page(bp);
   RecordDescriptor* rd = construct_record_descriptor();
 
-  Record r = record_init(RECORD_LEN);
+  int recordLength = compute_record_length(rd, person_id, firstName, lastName, age);
+  Record r = record_init(recordLength);
+
-  serialize_data(rd, r, person_id, name, age, dailySteps, distanceFromHome, isAlive);
+  serialize_data(rd, r, person_id, firstName, lastName, age);
-  bool insertSuccessful = page_insert(slot->pg, r, RECORD_LEN);
+  bool insertSuccessful = page_insert(slot->pg, r, recordLength);
 
   free_record_desc(rd);
   free(r);
   
   return insertSuccessful;
 }
```

```diff
 static bool analyze_selectstmt(SelectStmt* s) {
   for (int i = 0; i < s->targetList->length; i++) {
     ResTarget* r = (ResTarget*)s->targetList->elements[i].ptr;
     if (
       !(
         strcasecmp(r->name, "person_id") == 0 ||
-        strcasecmp(r->name, "name") == 0 ||
-        strcasecmp(r->name, "age") == 0 ||
-        strcasecmp(r->name, "daily_steps") == 0 ||
-        strcasecmp(r->name, "distance_from_home") == 0 ||
-        strcasecmp(r->name, "is_alive") == 0
+        strcasecmp(r->name, "first_name") == 0 ||
+        strcasecmp(r->name, "last_name") == 0 ||
+        strcasecmp(r->name, "age") == 0
       )
     ) {
       return false;
     }
   }
   return true;
 }
```

```diff
 int main(int argc, char** argv) {

*** code omitted for brevity ***

         break;
       case T_InsertStmt: {
         int32_t person_id = ((InsertStmt*)n)->personId;
-        char* name = ((InsertStmt*)n)->name;
-        uint8_t age = ((InsertStmt*)n)->age;
-        int16_t dailySteps = ((InsertStmt*)n)->dailySteps;
-        int64_t distanceFromHome = ((InsertStmt*)n)->distanceFromHome;
-        uint8_t isAlive = ((InsertStmt*)n)->isAlive;
+        char* firstName = ((InsertStmt*)n)->firstName;
+        char* lastName = ((InsertStmt*)n)->lastName;
+        int32_t age = ((InsertStmt*)n)->age;
-        if (!insert_record(bp, person_id, name, age, dailySteps, distanceFromHome, isAlive)) {
+        if (!insert_record(bp, person_id, firstName, lastName, age)) {
           printf("Unable to insert record\n");
         }
         break;
       }
       case T_SelectStmt:

*** code omitted for brevity ***
 
   return EXIT_SUCCESS;
 }
```

The rest of the changes are just accounting for the new columns in the table definition. All of the logic remains the same as before.