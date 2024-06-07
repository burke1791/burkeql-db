#include <string.h>

#include "system/syscmd.h"

CliSysCmd parse_syscmd(const char* cmd) {
  if (strcmp(cmd, "quit") == 0) return SYSCMD_QUIT;
  if (strcmp(cmd, "buf") == 0) return SYSCMD_BUFFER_SUMMARY;

  return SYSCMD_UNRECOGNIZED;
}