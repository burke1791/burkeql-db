#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#include "gram.tab.h"
#include "parser/parsetree.h"
#include "parser/parse.h"
#include "global/config.h"
#include "storage/file.h"
#include "storage/page.h"

Config* conf;

static void print_prompt() {
  printf("bql > ");
}

int main(int argc, char** argv) {
  // initialize global config
  conf = new_config();

  if (!set_global_config(conf)) {
    return EXIT_FAILURE;
  }

  // print config
  print_config(conf);

  FileDesc* fd = file_open(conf->dataFile, "wb");
  Page pg = read_page(fd->fp, 1);

  while(true) {
    print_prompt();
    Node* n = parse_sql();

    if (n == NULL) continue;

    print_node(n);

    switch (n->type) {
      case T_SysCmd:
        if (strcmp(((SysCmd*)n)->cmd, "quit") == 0) {
          free_node(n);
          printf("Shutting down...\n");
          flush_page(fd->fp, pg);
          free_page(pg);
          file_close(fd);
          return EXIT_SUCCESS;
        }
      case T_InsertStmt:
        // serialize data into a Record
        // insert the record on the page
    }

    free_node(n);
  }

  return EXIT_SUCCESS;
}