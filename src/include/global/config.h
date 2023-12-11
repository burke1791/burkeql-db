#ifndef CONFIG_H
#define CONFIG_H

#include <stdbool.h>

typedef enum ConfigParameter {
  CONF_DATA_FILE,
  CONF_PAGE_SIZE,
  CONF_UNRECOGNIZED
} ConfigParameter;

typedef struct Config {
  char* dataFile;
  int pageSize;
} Config;

Config* new_config();
void free_config(Config* conf);

bool set_global_config(Config* conf);

void print_config(Config* conf);

#endif /* CONFIG_H */