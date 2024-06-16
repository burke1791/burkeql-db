#include <string.h>

#include "system/syscmd.h"
#include "system/systable.h"
#include "system/syscolumn.h"
#include "system/syssequence.h"
#include "access/tableam.h"
#include "resultset/recordset.h"
#include "resultset/resultset_print.h"

CliSysCmd parse_syscmd(const char* cmd) {
  if (strcmp(cmd, "quit") == 0) return SYSCMD_QUIT;
  if (strcmp(cmd, "buf") == 0) return SYSCMD_BUFFER_SUMMARY;
  if (strcmp(cmd, "bufd") == 0) return SYSCMD_BUFFER_DETAILS;
  if (strcmp(cmd, "file") == 0) return SYSCMD_BUFFILE_SUMMARY;
  if (strcmp(cmd, "t") == 0) return SYSCMD_SYS_TABLE_TABLES;
  if (strcmp(cmd, "c") == 0) return SYSCMD_SYS_TABLE_COLUMNS;
  if (strcmp(cmd, "s") == 0) return SYSCMD_SYS_TABLE_SEQUENCES;

  return SYSCMD_UNRECOGNIZED;
}

static void syscmd_sys_table_tables(BufMgr* buf) {
  TableDesc* td = new_tabledesc("_tables");
  td->rd = systable_get_record_desc();
  RecordSet* rs = new_recordset();

  tableam_fullscan(buf, td, rs);
  resultset_print(td->rd, rs, td->rd);

  free_recordset(rs, td->rd);
  free_tabledesc(td);
}

static void syscmd_sys_table_columns(BufMgr* buf) {
  TableDesc* td = new_tabledesc("_columns");
  td->rd = syscolumn_get_record_desc();
  RecordSet* rs = new_recordset();

  tableam_fullscan(buf, td, rs);
  resultset_print(td->rd, rs, td->rd);

  free_recordset(rs, td->rd);
  free_tabledesc(td);
}

static void syscmd_sys_table_sequences(BufMgr* buf) {
  TableDesc* td = new_tabledesc("_sequences");
  td->rd = syssequence_get_record_desc();
  RecordSet* rs = new_recordset();

  tableam_fullscan(buf, td, rs);
  resultset_print(td->rd, rs, td->rd);

  free_recordset(rs, td->rd);
  free_tabledesc(td);
}

void run_syscmd(const char* cmd, BufMgr* buf) {
  switch (parse_syscmd(cmd)) {
    case SYSCMD_BUFFER_SUMMARY:
      bufmgr_diag_summary(buf);
      break;
    case SYSCMD_BUFFER_DETAILS:
      bufmgr_diag_details(buf);
      break;
    case SYSCMD_BUFFILE_SUMMARY:
      buffile_diag_summary(buf->fdl);
      break;
    case SYSCMD_SYS_TABLE_TABLES:
      syscmd_sys_table_tables(buf);
      break;
    case SYSCMD_SYS_TABLE_COLUMNS:
      syscmd_sys_table_columns(buf);
      break;
    case SYSCMD_SYS_TABLE_SEQUENCES:
      syscmd_sys_table_sequences(buf);
      break;
    case SYSCMD_UNRECOGNIZED:
      printf("Unrecognized system command\n");
  }
}