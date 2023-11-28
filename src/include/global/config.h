#ifndef CONFIG_H
#define CONFIG_H

#include <stdbool.h>

typedef enum ConfigParameter {
  CONF_DATA_FILE,
  CONF_UNRECOGNIZED
} ConfigParameter;

typedef struct Config {
  char* dataFile;
} Config;

Config* new_config();

bool set_global_config(Config* conf);

void free_config(Config* conf);

void print_config(Config* conf);

#endif /* CONFIG_H */