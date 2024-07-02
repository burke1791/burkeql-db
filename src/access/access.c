#include "access/access.h"

FilterList* new_filterlist() {
  FilterList* fl = new_linkedlist();
  return fl;
}


void filterlist_append_filter(FilterList* fl, FilterType* type, FilterValue* left, FilterValue* right) {
  Filter* f = malloc(sizeof(Filter));
  f->type = type;
  f->left = left;
  f->right = right;

  linkedlist_append(fl, f);
}


FilterValue* new_filtervalue(FilterValueType type, DataType dataType, FilterConstant* constant, TargetEntry* col) {
  FilterValue* fv = malloc(sizeof(FilterValue));

  fv->type = type;
  fv->dataType = dataType;
  fv->constant = constant;
  fv->col = col;

  return fv;
}