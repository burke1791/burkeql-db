#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <stdbool.h>

#include "resultset/resultset_print.h"
#include "storage/datum.h"

static int num_digits(int64_t num) {
  if (num < 0) return num_digits(-num) + 1; // extra '1' is to account for the negative sign in the display
  if (num < 10) return 1;
  return 1 + num_digits(num / 10);
}

static void compute_column_widths(RecordDescriptor* rd, RecordSet* rs, int* widths) {
  int maxLen;

  for (int i = 0; i < rd->ncols; i++) {
    maxLen = strlen(rd->cols[i].colname);
    Column* col = &rd->cols[i];
    ListItem* row = rs->rows->head;

    int count = 0;
    while (row != NULL) {
      count++;
      int len;
      RecordSetRow* data = (RecordSetRow*)row->ptr;
      // Datum* values = (Datum*)row->ptr;

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

static int get_col_index(RecordDescriptor* rd, char* name) {
  for (int i = 0; i < rd->ncols; i++) {
    if (strcasecmp(rd->cols[i].colname, name) == 0) return rd->cols[i].colnum;
  }

  return -1;
}

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