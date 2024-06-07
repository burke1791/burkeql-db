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

typedef enum CliSysCmd {
  SYSCMD_QUIT,
  SYSCMD_BUFFER_SUMMARY,
  SYSCMD_UNRECOGNIZED
} CliSysCmd;

CliSysCmd parse_syscmd(const char* cmd);

#endif /* SYSCMD_H */