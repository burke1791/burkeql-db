# Storage and Fill

As has been the case for each new data type, we need to change the definition of the table we're working with. For this `Varchar` section, our table looks like this:

```sql
Create Table person (
    person_id Int Not Null,
    first_name Varchar(20) Not Null,
    last_name Varchar(20) Not Null,
    age Int Not Null
);
```

The only purpose of that `age` column is to demonstrate how the storage engine physically stores fixed-length columns before variable-length columns. But more on that later.

Fortunately, we don't have to refactor the code too much in order to support Varchars, but there is some nuance to the `fill` and `defill` logic that might take a bit to wrap your head around. We'll get to that later though.

## Storage Engine

First up, we start with the simple changes: supporting a new data type.

`src/include/storage/record.h`

```diff
 typedef enum DataType {
   DT_TINYINT,     /* 1-byte, unsigned */
   DT_SMALLINT,    /* 2-bytes, signed */
   DT_INT,         /* 4-bytes, signed */
   DT_BIGINT,      /* 8-bytes, signed */
   DT_BOOL,        /* 1-byte, unsigned | similar to DT_TINYINT, but always evaluates to 1 or 0 */
   DT_CHAR,        /* Byte-size defined at table creation */
+  DT_VARCHAR,     /* Variable length. A 2-byte "header" stores the length of the column
+                     followed by the actual column bytes */
   DT_UNKNOWN
 } DataType;
```

Next, since our storage engine separates the column data into two sections: fixed-length and variable-length, we need to add some metadata to support that downstream logic.

`src/include/storage/record.h`

```diff
 typedef struct RecordDescriptor {
   int ncols;        /* number of columns (defined by the Create Table DDL) */
+  int nfixed;       /* number of fixed-length columns */
   Column cols[];
 } RecordDescriptor;
```

We add a property to the `RecordDescriptor` to keep track of the number of fixed-length columns in the record. Since this struct is passed all over the place in the storage engine, we'll be able to use it to accurately read and write column data in the correct spot.

`src/include/storage/record.h`

```diff
-void fill_record(RecordDescriptor* rd, Record r, Datum* data);
+void fill_record(RecordDescriptor* rd, Record r, Datum* fixed, Datum* varlen);
```

We also need to change the definition of the `fill_record` function to support the two different sections in the physical record structure. Since we need to write all fixed-length data first, then write the variable-length data, it'll be easier to work with them in separate variables.

## Fill

Let's step through the `fill_record` function and refactor the code as needed. Since separating fixed-length and variable-length columns is such a fundamental change, the `fill_record` function is going to be a complete rewrite. So I'm not providing a diff.

`src/storage/record.c`

```c
void fill_record(RecordDescriptor* rd, Record r, Datum* fixed, Datum* varlen) {
  // fill fixed-length columns
  for (int i = 0; i < rd->nfixed; i++) {
    Column* col = get_nth_col(rd, true, i);

    fill_val(col, &r, fixed[i]);
  }

  // fill varlen columns
  for (int i = 0; i < (rd->ncols - rd->nfixed); i++) {
    Column* col = get_nth_col(rd, false, i);

    fill_val(col, &r, varlen[i]);
  }
}
```

The logic inside each for loop is pretty much the same, but now we have two loops. First, we write the fixed-length columns to the record, then we write the variable-length columns. And because the columns in the `RecordDescriptor` can be in a different order than the storage engine writes them, we need a helper function get us the correct `Column` from the `RecordDescriptor`.

`src/storage/record.c`

```c
static Column* get_nth_col(RecordDescriptor* rd, bool isFixed, int n) {
  int nCol = 0;

  for (int i = 0; i < rd->ncols; i++) {
    if (rd->cols[i].dataType == DT_VARCHAR) {
      // column is variable-length
      if (!isFixed && nCol == n) return &rd->cols[i];
      if (!isFixed) nCol++;
    } else {
      // column is fixed length
      if (isFixed && nCol == n) return &rd->cols[i];
      if (isFixed) nCol++;
    }
  }

  return NULL;
}
```

This function is fairly straightforward. We just loop through the `RecordDescriptor`'s `Column` array and return the `n`th column according to the `isFixed` parameter. Although pretty simple, this function is crucial to reconciling the different physical ordering of columns compared to the order in the `Create Table` statement.

Next, let's step into `fill_val`.

`src/storage/record.c`

```diff
 static void fill_val(Column* col, char** dataP, Datum datum) {
   int16_t dataLen;
   char* data = *dataP;
   
   switch (col->dataType) {

*** code omitted for brevity ***
 
       free(str);
       break;
+    case DT_VARCHAR:
+      fill_varchar(col, data, &dataLen, datum);
+      break;
   }
 
   data += dataLen;
   *dataP = data;
 }
```

We just need to add a new `case` to the switch/case block and call our new `fill_varchar` function (defined below).

`src/storage/record.c`

```c
static void fill_varchar(Column* col, char* data, int16_t* dataLen, Datum value) {
  int16_t charLen = strlen(datumGetString(value));
  if (charLen > col->len) charLen = col->len;
  charLen += 2; // account for the 2-byte length overhead
  
  // write the 2-byte length overhead
  memcpy(data, &charLen, 2);

  // write the actual data
  memcpy(data + 2, datumGetString(value), charLen - 2);
  *dataLen = charLen;
}
```

This logic is pretty similar to the `DT_CHAR` case, except we need to also write the 2-byte length overhead directly before the column data itself. And remember, the length overhead also includes the 2-byte length of itself, hence the `charLen += 2;` statement.

And that does it for the "fill" part of the storage engine. Next, we'll muddle through the tedious task of updating our temporary code - we are so close to ditching the temp code for good!