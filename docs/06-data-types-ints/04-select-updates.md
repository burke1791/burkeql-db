# Select Updates

The last thing we need to do is refactor our resultset code to handle the new data types. These changes are very similar to the defill updates - simply adding swicth/cases for the three new `Int` types. We have three functions to update:

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
+        case DT_TINYINT:
+          len = num_digits(datumGetUInt8(data->values[i]));
+          break;
+        case DT_SMALLINT:
+          len = num_digits(datumGetInt16(data->values[i]));
+          break;
         case DT_INT:
           len = num_digits(datumGetInt32(data->values[i]));
           break;
+        case DT_BIGINT:
+          len = num_digits(datumGetInt64(data->values[i]));
+          break;
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
+    case DT_TINYINT:
+      numDigits = num_digits(datumGetUInt8(d));
+      cell = malloc(numDigits + 1);
+      sprintf(cell, "%u", datumGetUInt8(d));
+      break;
+    case DT_SMALLINT:
+      numDigits = num_digits(datumGetInt16(d));
+      cell = malloc(numDigits + 1);
+      sprintf(cell, "%d", datumGetInt16(d));
+      break;
     case DT_INT:
       numDigits = num_digits(datumGetInt32(d));
       cell = malloc(numDigits + 1);
       sprintf(cell, "%d", datumGetInt32(d));
       break;
+    case DT_BIGINT:
+      numDigits = num_digits(datumGetInt64(d));
+      cell = malloc(numDigits + 1);
+      sprintf(cell, "%ld", datumGetInt64(d));
+      break;
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
+        case DT_TINYINT:
+        case DT_SMALLINT:
         case DT_INT:
+        case DT_BIGINT:
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

Pretty basic here. We're just adding logic for handling each of the new data types.

Now let's try to select some data:

```shell
$ make clean && make && ./burkeql
======   BurkeQL Config   ======
= DATA_FILE: /home/burke/source_control/burkeql-db/db_files/main.dbd
= PAGE_SIZE: 128
bql > select person_id, name, age, daily_steps, distance_from_home;
======  Node  ======
=  Type: Select
=  Targets:
=    person_id
=    name
=    age
=    daily_steps
=    distance_from_home
--------
*** Rows: 1
--------
|person_id |name        |age |daily_steps |distance_from_home |
---------------------------------------------------------------
|        69|Chris Burke |  45|       12345|        12345678900|
(Rows: 1)

bql >
```

We can see our resultset code can now gracefully handle the new columns and is accurately reporting the same data we inserted earlier.