/**
 * @file syscmd.h
 * @author Chris Burke (you@domain.com)
 * @brief 
 * @version 0.1
 * @date 2024-06-06
 * 
 * @copyright Copyright (c) 2024
 * 
 * API for the bql system commands
 */

#ifndef SYSCMD_H
#define SYSCMD_H

#include "buffer/bufmgr.h"

typedef enum CliSysCmd {
  SYSCMD_QUIT,
  SYSCMD_BUFFER_SUMMARY,
  SYSCMD_BUFFER_DETAILS,
  SYSCMD_BUFFILE_SUMMARY,
  SYSCMD_SYS_TABLE_TABLES,
  SYSCMD_SYS_TABLE_COLUMNS,
  SYSCMD_SYS_TABLE_SEQUENCES,
  SYSCMD_UNRECOGNIZED
} CliSysCmd;

CliSysCmd parse_syscmd(const char* cmd);
void run_syscmd(const char* cmd, BufMgr* buf);

#endif /* SYSCMD_H */