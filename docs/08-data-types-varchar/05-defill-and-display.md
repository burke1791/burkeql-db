# Defill and Display

The final piece to our `Varchar` journey is updating our defill and display code so we can run select statements.

## Defill

`src/storage/record.c`

```diff
 void defill_record(RecordDescriptor* rd, Record r, Datum* values) {
   int offset = sizeof(RecordHeader);
 
+  Column* col;
   for (int i = 0; i < rd->ncols; i++) {
-    Column* col = &rd->cols[i];
-    values[i] = record_get_col_value(col, r, &offset);
+    if (i < rd->nfixed) {
+      col = get_nth_col(rd, true, i);
+    } else {
+      col = get_nth_col(rd, false, i - rd->nfixed);
+    }
+
+    values[col->colnum] = record_get_col_value(col, r, &offset);
   }
 }
```

We're essentially fully rewriting the logic to gracefully work with the fixed/varlen column separation on disk. We loop through each column and utilize our `get_nth_col` function to make sure we're grabbing the correct information.

`src/storage/record.c`

```diff
 static Datum record_get_col_value(Column* col, Record r, int* offset) {
   switch (col->dataType) {
     case DT_BOOL:     // Bools and TinyInts are the same C-type
     case DT_TINYINT:
       return record_get_tinyint(r, offset);
     case DT_SMALLINT:
       return record_get_smallint(r, offset);
     case DT_INT:
       return record_get_int(r, offset);
     case DT_BIGINT:
       return record_get_bigint(r, offset);
     case DT_CHAR:
       return record_get_char(r, offset, col->len);
+    case DT_VARCHAR:
+      return record_get_varchar(r, offset);
     default:
       printf("record_get_col_value() | Unknown data type!\n");
       return (Datum)NULL;
   }
 }
```

Add a new case statement for the `Varchar` type.

`src/storage/record.c`

```c
static Datum record_get_varchar(Record r, int* offset) {
  int16_t len;
  memcpy(&len, r + *offset, 2);

  /*
    The `len - 1` expression is a combination of two steps:

    We subtract 2 bytes because we don't need memory for
    the 2-byte length overhead.

    Then we add 1 byte to account for the null terminator we
    need to append on the end of the string.
  */
  char* pChar = malloc(len - 1);
  memcpy(pChar, r + *offset + 2, len - 2);
  pChar[len - 2] = '\0';
  *offset += len;
  return charGetDatum(pChar);
}
```

Create a new function to extract the varchar data.

## Display

`src/resultset/resultset_print.c`

```diff
 static void compute_column_widths(RecordDescriptor* rd, RecordSet* rs, int* widths) {
   int maxLen;
 
   for (int i = 0; i < rd->ncols; i++) {
     maxLen = strlen(rd->cols[i].colname);
     Column* col = &rd->cols[i];
     ListItem* row = rs->rows->head;
 
     while (row != NULL) {
       int len;
       RecordSetRow* data = (RecordSetRow*)row->ptr;
 
       switch (col->dataType) {
         case DT_BOOL:
         case DT_TINYINT:
           len = num_digits(datumGetUInt8(data->values[i]));
           break;
         case DT_SMALLINT:
           len = num_digits(datumGetInt16(data->values[i]));
           break;
         case DT_INT:
           len = num_digits(datumGetInt32(data->values[i]));
           break;
         case DT_BIGINT:
           len = num_digits(datumGetInt64(data->values[i]));
           break;
         case DT_CHAR:
+        case DT_VARCHAR:
           len = strlen(datumGetString(data->values[i]));
           break;
         default:
           printf("compute_column_widths() | Unknown data type\n");
       }
 
       if (len > maxLen) maxLen = len;
       row = row->next;
     }
 
     widths[i] = maxLen + 1;
   }
 }
```

`src/resultset/resultset_print.c`

```diff
 void resultset_print(RecordDescriptor* rd, RecordSet* rs, RecordDescriptor* targets) {
   printf("--------\n");
   printf("*** Rows: %d\n", rs->rows->numItems);
   printf("--------\n");
 
   if (rs->rows->numItems == 0) return;
 
   int* widths = malloc(sizeof(int) * rd->ncols);
   compute_column_widths(rd, rs, widths);
   print_column_headers(rd, targets, widths);
 
   ListItem* row = rs->rows->head;
   while (row != NULL) {
     printf("|");
     Datum* values = (Datum*)(((RecordSetRow*)row->ptr)->values);
     for (int i = 0; i < targets->ncols; i++) {
       int colIndex = get_col_index(rd, targets->cols[i].colname);
       Column* col = &rd->cols[colIndex];
       
       switch (col->dataType) {
         case DT_BOOL:
         case DT_TINYINT:
         case DT_SMALLINT:
         case DT_INT:
         case DT_BIGINT:
           print_cell_num(col->dataType, values[colIndex], widths[colIndex]);
           break;
         case DT_CHAR:
+        case DT_VARCHAR:
           print_cell_with_padding(datumGetString(values[colIndex]), widths[colIndex], false);
           break;
         default:
           printf("resultset_print() | Unknown data type\n");
       }
     }
     printf("\n");
     row = row->next;
   }
 
   printf("(Rows: %d)\n\n", rs->rows->numItems);
 
   free(widths);
 }
```

No new logic necessary, we just need to add `DT_VARCHAR` to the switch/case blocks and piggy back off of the `DT_CHAR` logic.

`src/resultset/recordset.c`

```diff
 static void free_recordset_row_columns(RecordSetRow* row, RecordDescriptor* rd) {
   for (int i = 0; i < rd->ncols; i++) {
-    if (rd->cols[i].dataType == DT_CHAR) {
+    if (rd->cols[i].dataType == DT_CHAR || rd->cols[i].dataType == DT_VARCHAR) {
       free((void*)row->values[i]);
     }
   }
 }
```

Lastly, we just need to make sure we free the memory associated with `DT_VARCHAR` values.

## Running Select Statements

Now we can test our changes by running select statements:

```shell
$ ./burkeql
======   BurkeQL Config   ======
= DATA_FILE: /home/burke/source_control/burkeql-db/db_files/main.dbd
= PAGE_SIZE: 128
bql > select person_id, first_name, last_name, age;
======  Node  ======
=  Type: Select
=  Targets:
=    person_id
=    first_name
=    last_name
=    age
--------
*** Rows: 2
--------
|person_id |first_name           |last_name         |age |
----------------------------------------------------------
|         1|Chris                |Burke             |  33|
|         2|I am longer than twe |was it truncated? |  69|
(Rows: 2)

bql >
```

Success! Our resultset display is functioning properly.

In the next section, we're going to introduce `Null` columns to our database. Don't worry though, the columns in our table will stay the same so the temporary code refactor will be minimal.