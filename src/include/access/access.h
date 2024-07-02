#ifndef ACCESS_H
#define ACCESS_H

#include "utility/linkedlist.h"
#include "parser/parsetree.h"
#include "storage/record.h"
#include "parser/querytree.h"

/**
 * @brief Need to make a generic filter struct and functionality. We can restrict it to simple
 * use-cases for the time being: Number comparison and String equivalency checks.
 * 
 * 
 * 
 */

typedef struct LinkedList FilterList;

typedef enum FilterType {
  FLTR_EQ,    /* the only one allowed when comparing strings */
  FLTR_GT,
  FLTR_GTE,
  FLTR_LT,
  FLTR_LTE
} FilterType;

typedef enum FilterValueType {
  FLTR_VAL_COLUMN,
  FLTR_VAL_LITERAL
} FilterValueType;

typedef struct FilterConstant {
  int64_t intval;
  char* str;
} FilterConstant;

typedef struct FilterValue {
  FilterValueType type;
  DataType dataType;
  FilterConstant* constant;
  TargetEntry* col;
} FilterValue;

typedef struct Filter {
  FilterType type;
  FilterValue* left;
  FilterValue* right;
} Filter;

FilterList* new_filterlist();
void filterlist_append_filter(FilterList* fl, FilterType* type, FilterValue* left, FilterValue* right);
FilterValue* new_filtervalue(FilterValueType type, DataType dataType, FilterConstant* constant, TargetEntry* col);
void free_filterlist();

#endif /* ACCESS_H */