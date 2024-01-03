# Select Updates

These changes are extremely simple. We're just going to piggy-back off the `DT_TINYINT` logic like we've been doing this whole time.

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
+        case DT_BOOL:
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

```diff
 static void print_cell_num(DataType dt, Datum d, int width) {
   int numDigits;
   char* cell;
 
   switch (dt) {
+    case DT_BOOL:
     case DT_TINYINT:
       numDigits = num_digits(datumGetUInt8(d));
       cell = malloc(numDigits + 1);
       sprintf(cell, "%u", datumGetUInt8(d));
       break;
     case DT_SMALLINT:
       numDigits = num_digits(datumGetInt16(d));
       cell = malloc(numDigits + 1);
       sprintf(cell, "%d", datumGetInt16(d));
       break;
     case DT_INT:
       numDigits = num_digits(datumGetInt32(d));
       cell = malloc(numDigits + 1);
       sprintf(cell, "%d", datumGetInt32(d));
       break;
     case DT_BIGINT:
       numDigits = num_digits(datumGetInt64(d));
       cell = malloc(numDigits + 1);
       sprintf(cell, "%ld", datumGetInt64(d));
       break;
   }
 
   cell[numDigits] = '\0';
   print_cell_with_padding(cell, width, true);
 
   if (cell != NULL) free(cell);
 }
```

```diff
 void resultset_print(RecordDescriptor* rd, RecordSet* rs, RecordDescriptor* targets) {
   
 *** omitted for brevity ***
 
   while (row != NULL) {
     printf("|");
     Datum* values = (Datum*)(((RecordSetRow*)row->ptr)->values);
     for (int i = 0; i < targets->ncols; i++) {
       int colIndex = get_col_index(rd, targets->cols[i].colname);
       Column* col = &rd->cols[colIndex];
       
       switch (col->dataType) {
+        case DT_BOOL:
         case DT_TINYINT:
         case DT_SMALLINT:
         case DT_INT:
         case DT_BIGINT:
           print_cell_num(col->dataType, values[colIndex], widths[colIndex]);
           break;
         case DT_CHAR:
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

Now let's test it out:

```shell
$ make clean && make && ./burkeql
======   BurkeQL Config   ======
= DATA_FILE: /home/burke/source_control/burkeql-db/db_files/main.dbd
= PAGE_SIZE: 128
bql > select name, is_alive;
======  Node  ======
=  Type: Select
=  Targets:
=    name
=    is_alive
--------
*** Rows: 1
--------
|name        |is_alive |
------------------------
|Chris Burke |        1|
(Rows: 1)

bql >
```

We're just selecting the `name` and `is_alive` columns, but our code is correctly displaying the `Bool` data type.