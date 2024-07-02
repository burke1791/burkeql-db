#include <strings.h>

#include "buffer/bufmgr.h"
#include "parser/analyze.h"
#include "parser/parsetree.h"
#include "utility/linkedlist.h"
#include "storage/table.h"
#include "system/systable.h"
#include "access/tableam.h"

/**
 * @brief Checks the system tables to ensure all tables referenced
 * in the `fromClause` exist in the database
 * 
 * @param fromClause 
 * @return true 
 * @return false 
 */
static bool analyze_selectstmt_tables(BufMgr* buf, ParseList* fromClause) {
  TableDesc* td = new_tabledesc("_tables");
  td->rd = systable_get_record_desc();

  RecordSet* rs = new_recordset();

  tableam_fullscan(buf, td, rs);

  bool exists = false;
  for (int i = 0; i < fromClause->length; i++) {
    exists = false;
    TableRef* t = fromClause->elements[i].ptr;

    ListItem* li = rs->rows->head;
    
    while (li != NULL && !exists) {
      RecordSetRow* row = li->ptr;

      if (strcasecmp(t->name, datumGetString(row->values[1])) == 0) {
        exists = true;
      }

      li = li->next;
    }

    if (!exists) break;
  }

  free_recordset(rs, td->rd);
  free_tabledesc(td);

  return exists;
}

/**
 * @brief Checks the system tables to ensure all columns referenced in
 * `targetList` exist in one of the tables in the `fromClause`.
 * 
 * @todo check for ambiguous columns - need to update the parser to
 * allow referencing columns with two-part naming first.
 * 
 * @todo expand the '*' into all columns in all tables in the `fromClause`
 * 
 * @param fromClause 
 * @param targetList 
 * @return true 
 * @return false 
 */
static bool analyze_selectstmt_table_columns(BufMgr* buf, ParseList* fromClause, ParseList* targetList) {
  return true;
}

/**
 * @brief Walks the SelectStmt parsetree and ensures the query is
 * semantically sound. I.e. referenced tables/columns exist, data
 * types play nice with each other, etc.
 * 
 * @param tree 
 * @return true 
 * @return false 
 */
static bool analyze_selectstmt(BufMgr* buf, Node* tree) {
  SelectStmt* s = (SelectStmt*)tree;

  if (s->fromClause != NULL && !analyze_selectstmt_tables(buf, s->fromClause)) {
    printf("referenced table does not exist\n");
    return false;
  }

  if (!analyze_selectstmt_table_columns(buf, s->fromClause, s->targetList)) {
    printf("referenced columns do not exist in the referenced tables\n");
    return false;
  }

  return true;
}

bool analyze_parsetree(BufMgr* buf, Node* tree) {
  switch (tree->type) {
    case T_SelectStmt:
      return analyze_selectstmt(buf, tree);
    default:
      return true;
  }
}