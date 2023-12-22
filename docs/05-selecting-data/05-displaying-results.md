# Displaying Results

In this section, we're going to write the code that parses through the data returned by the table scan function and displays them in a pretty way on the terminal. Something like this:

```shell
$ make && ./burkeql
======   BurkeQL Config   ======
= DATA_FILE: /home/burke/source_control/burkeql-db/db_files/main.dbd
= PAGE_SIZE: 128
bql > select person_id, name;
======  Node  ======
=  Type: Select
=  Targets:
=    person_id
=    name
--------
*** Rows: 2
--------
|person_id |name         |
--------------------------
|        69|chris burke  |
|        77|hello, world |
(Rows: 2)

bql >
```

When we run `select person_id, name;`, the "backend" performs the table scan and hands the data back to the "frontend". We then parse through everything and determine how wide each column needs to be in order for the tabular format to look pretty.

## Resultset Header

`src/include/resultset/resultset_print.h`

```c
#ifndef RESULTSET_PRINT_H
#define RESULTSET_PRINT_H

#include "resultset/recordset.h"

void resultset_print(RecordDescriptor* rd, RecordSet* rs, RecordDescriptor* targets);

#endif /* RESULTSET_PRINT_H */
```

I created a dedicated header file for outputting query results to the console. Right now it just has a single function that encapsulates all the work required to pretty-print our results. We send it a `RecordDescriptor* rd` that describes all columns in our table, a `RecordSet* rs` that contains all of the data brought back by the table scan, and a `RecordDescriptor* targets` that has an entry for each column we want to display.

Remember, the table scan function pull back all columns regardless of what our select statement says it wants. So we need this additional `RecordDescriptor` to tell the print function exactly what we want displayed. It also works the other way, e.g. if we run `select person_id, name, person_id, name;`, the print function will know it needs to display each column twice.

## Resultset Print Code

This is a rather long file, so bear with me.

`src/resultset/resultset_print.c`

```c
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
        case DT_INT:
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

Starting with the function we expose in the header file, we print a little row count header at the top. And if there aren't any rows to display, we return early.

Next we allocate memory for an int array with an entry for each column in our table. Then we offload the computation of those widths to our helper function `compute_column_widths`:

`src/resultset/resultset_print.c`

```c
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
        case DT_INT:
          len = num_digits(datumGetInt32(data->values[i]));
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

In this function, we have a nested loop that calculates the largest width for a given column across all rows. It adds 1 to the `maxLen` and sets the value in our `widths` array. The reason for the additional padding is just so the table display doesn't feel to congested.

For `DT_INT`, we calculate the number of digits to get the maxLen, and for `DT_CHAR` we use the built in `strlen` function. The `num_digits` function is defined as follows:

`src/resultset/resultset_print.c`

```c
static int num_digits(int64_t num) {
  if (num < 0) return num_digits(-num) + 1; // extra '1' is to account for the negative sign in the display
  if (num < 10) return 1;
  return 1 + num_digits(num / 10);
}
```

Pretty basic recursive function where we add 1 each time we divide the number by 10.

Now back to the primary print function. After we compute the column widths, we call `print_column_headers`:

`src/resultset/resultset_print.c`

```c
static void print_column_headers(RecordDescriptor* rd, RecordDescriptor* rs, int* widths) {
  printf("|");

  int totalWidth = 1;
  for (int i = 0; i < rs->ncols; i++) {
    int colIndex = get_col_index(rd, rs->cols[i].colname);
    print_cell_with_padding(rs->cols[i].colname, widths[colIndex], false);
    totalWidth += (widths[colIndex] + 1);
  }

  printf("\n");

  for (int i = 0; i < totalWidth; i++) {
    printf("-");
  }
  printf("\n");
}
```

We pass in the `RecordDescriptor` for the table, the `RecordDescriptor` for our target list, and the widths array. We start by printing the column boundary "pipe" character, then we loop through the columns in our target list. Based on the `colname` of the **target** column, we find its location in the `RecordDescriptor` for the table, which is also the same array index in the widths array. Using this information, we make a call to `print_cell_with_padding` to output our column name and update our `totalWidth` tracker.

Finally at the end, we print a horizontal row of dashes to separate our column headers from the row data.

Let's take a look at the two helper functions we used in printing out the column headers:

`src/resultset/resultset_print.c`

```c
static int get_col_index(RecordDescriptor* rd, char* name) {
  for (int i = 0; i < rd->ncols; i++) {
    if (strcasecmp(rd->cols[i].colname, name) == 0) return rd->cols[i].colnum;
  }

  return -1;
}
```

We loop through the table's `RecordDescriptor` and just do a basic case-insensitive string comparison to find the correct column index.

```c
static void print_cell_with_padding(char* cell, int cellWidth, bool isRightAligned) {
  int padLen = cellWidth - strlen(cell);

  if (padLen < 0) {
    printf("\npadLen: %d\ncellWidth: %d\n valWidth: %ld\n", padLen, cellWidth, strlen(cell));
    return;
  }

  if (padLen == 0) {
    printf("%s|", cell);
  } else if (isRightAligned) {
    printf("%*s%s|", padLen, " ", cell);
  } else {
    printf("%s%*s|", cell, padLen, " ");
  }
}
```

And here we compute how much padding we need by subtracting the `strlen` of the value we want to print from the previously computed `cellWidth`. If it's less than zero, we print a sort of error message. If there's no padding, we print the value as-is and immediately follow it with a column divider.

Next, we check if the caller wants the value right-aligned. This just determines whether we print the padding on the left or right side of the value. And instead of using a loop to print the padding, we use a clever trick of `printf`. The `%*s` will print `n` number of `s`. So in the right-align branch, we print `%*s%s`, meaning we print `padLen` number of `" "` (space characters) followed by the value of `cell`.

Now, continuing along our primary print function, we enter the looping mechanism that goes row-by-row and prints out each column we declare in our target list. Here's the loop code for reference:

`src/resultset/resultset_print.c` (`resultset_print()`)

```c
  ListItem* row = rs->rows->head;
  while (row != NULL) {
    printf("|");
    Datum* values = (Datum*)(((RecordSetRow*)row->ptr)->values);
    for (int i = 0; i < targets->ncols; i++) {
      int colIndex = get_col_index(rd, targets->cols[i].colname);
      Column* col = &rd->cols[colIndex];
      
      switch (col->dataType) {
        case DT_INT:
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
```

We start at the head of the linked list containing our data and extract the `Datum` array from the `ListItem`. Then we loop over each column in our target list and print out the corresponding value. For `DT_CHAR`, we can simply call the same function we went over above, but for `DT_INT` we need another helper function.

`src/resultset/resultset_print.c`

```c
static void print_cell_num(DataType dt, Datum d, int width) {
  int numDigits;
  char* cell;

  switch (dt) {
    case DT_INT:
      numDigits = num_digits(datumGetInt32(d));
      cell = malloc(numDigits + 1);
      sprintf(cell, "%d", datumGetInt32(d));
      break;
  }

  cell[numDigits] = '\0';
  print_cell_with_padding(cell, width, true);

  if (cell != NULL) free(cell);
}
```

Here we simply compute the number of digits, turn the number into a `char*`, then call `print_cell_with_padding`.

## Updating Main

We've covered all the code required to print our query results to the console, now let's update `main.c` and run the program.

`src/main.c`

```diff
 #include "gram.tab.h"
 #include "parser/parsetree.h"
 #include "parser/parse.h"
 #include "global/config.h"
 #include "storage/file.h"
 #include "storage/page.h"
+#include "storage/table.h"
 #include "buffer/bufpool.h"
+#include "storage/table.h"
+#include "resultset/recordset.h"
+#include "resultset/resultset_print.h"
+#include "access/tableam.h"
+#include "utility/linkedlist.h"

*** code removed for brevity ***

+static RecordDescriptor* construct_record_descriptor_from_target_list(ParseList* targetList) {
+  RecordDescriptor* rd = malloc(sizeof(RecordDescriptor) + (targetList->length * sizeof(Column)));
+  rd->ncols = targetList->length;
+
+  for (int i = 0; i < rd->ncols; i++) {
+    ResTarget* t = (ResTarget*)targetList->elements[i].ptr;
+
+    // we don't care about the data type or length here
+    construct_column_desc(&rd->cols[i], t->name, DT_UNKNOWN, i, 0);
+  }
+
+  return rd;
+}
```

Starting with a bunch of new include statements and this brand new function in our "temporary" section of code. We loop through the list of select targets generated by the parser and create a `RecordDescriptor` for use elsewhere. Notice we're supplying `DT_UNKNOWN` to the column constructor. It's because the data type in these column structs is irrelevant - in reality, we're just shoehorning the `RecordDescriptor` into a role it's not meant to play. But it's good enough for now.

We need to add the new data type to the `DataType` enum:

`src/include/storage/record.h`

```diff
 typedef enum DataType {
   DT_INT,     /* 4-bytes, signed */
   DT_CHAR,    /* Byte-size defined at table creation */
+  DT_UNKNOWN
 } DataType;
```

And finally, we have the logic that kicks off the select query:

`src/main.c`

```diff
     switch (n->type) {
       case T_SysCmd:
         if (strcmp(((SysCmd*)n)->cmd, "quit") == 0) {
           free_node(n);
           printf("Shutting down...\n");
           bufpool_flush_all(bp);
           bufpool_destroy(bp);
           file_close(fdesc);
           return EXIT_SUCCESS;
         }
         break;
       case T_InsertStmt:
         int32_t person_id = ((InsertStmt*)n)->personId;
         char* name = ((InsertStmt*)n)->name;
         if (!insert_record(bp, person_id, name)) {
           printf("Unable to insert record\n");
         }
         break;
       case T_SelectStmt:
         if (!analyze_node(n)) {
           printf("Semantic analysis failed\n");
+        } else {
+          TableDesc* td = new_tabledesc("person");
+          td->rd = construct_record_descriptor();
+          RecordSet* rs = new_recordset();
+          rs->rows = new_linkedlist();
+          RecordDescriptor* targets = construct_record_descriptor_from_target_list(((SelectStmt*)n)->targetList);
+          
+          tableam_fullscan(bp, td, rs->rows);
+          resultset_print(td->rd, rs, targets);
+
+          free_recordset(rs, td->rd);
+          free_tabledesc(td);
+          free_record_desc(targets);
+        }
         break;
     }
```

Essentially, if the analyzer says we're good, we run a full table scan. We start by allocating a `TableDesc`, a `RecordSet`, and a `RecordDescriptor` of our target list. Then we pass them to our table scan and print functions to take care of the heavy lifting. After that, we free up the memory we no longer need.

Lastly, we need to update our `Makefile`:

`src/Makefile`

```diff
SRC_FILES = main.c \
			parser/parse.c \
			parser/parsetree.c \
			global/config.c \
			storage/file.c \
			storage/page.c \
			storage/record.c \
			storage/datum.c \
-           buffer/bufpool.c \                        
+			storage/table.c \
+			buffer/bufpool.c \
+			access/tableam.c \
+			resultset/recordset.c \
+			resultset/resultset_print.c \
+			utility/linkedlist.c
```

Note the red line for `bufpool.c` is just because we moved it to a different location - it's still in the list.

## Running The Program

I'm starting with a brand new database file by deleting the existing one (if necessary). Run

```shell
$ rm db_files/main.dbd
$ make clean && make && ./burkeql
======   BurkeQL Config   ======
= DATA_FILE: /home/burke/source_control/burkeql-db/db_files/main.dbd
= PAGE_SIZE: 128
bql > insert 69 'chris burke';
Bytes read: 0
bql > insert 123456789 'Large number';
bql > select person_id, name;
--------
*** Rows: 2
--------
|person_id |name         |
--------------------------
|        69|chris burke  |
| 123456789|Large number |
(Rows: 2)

bql >
```

I removed the node outputs to reduce the noise in the console display. After startup, I insert two records then run a select on both columns, which shows exactly what we expect in the tabular form. What if we select the columns more than once?

```shell
bql > select person_id, name, PERSON_ID, NAME;
--------
*** Rows: 2
--------
|person_id |name         |PERSON_ID |NAME         |
---------------------------------------------------
|        69|chris burke  |        69|chris burke  |
| 123456789|Large number | 123456789|Large number |
(Rows: 2)

bql >
```

We get all of our target columns displayed to us - even the headers preserve the casing we used in our select statement.

That wraps up this section. In the next section we're going to start expanding the data types we support - starting with the `Int` class.