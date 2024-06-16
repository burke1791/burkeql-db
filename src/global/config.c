#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "global/config.h"

Config* new_config() {
  Config* conf = malloc(sizeof(Config));
  return conf;
}

void free_config(Config* conf) {
  if (conf->dataFile != NULL) free(conf->dataFile);
  free(conf);
}

void print_config(Config* conf) {
  printf("======   BurkeQL Config   ======\n");
  printf("= DATA_FILE:    %s\n", conf->dataFile);
  printf("= PAGE_SIZE:    %d\n", conf->pageSize);
  printf("= BUFPOOL_SIZE: %d\n", conf->bufpoolSize);
}

static ConfigParameter parse_config_param(char* p) {
  if (strcmp(p, "DATA_FILE") == 0) return CONF_DATA_FILE;
  if (strcmp(p, "PAGE_SIZE") == 0) return CONF_PAGE_SIZE;
  if (strcmp(p, "BUFPOOL_SIZE") == 0) return CONF_BUFPOOL_SIZE;

  return CONF_UNRECOGNIZED;
}

static void set_config_value(Config* conf, ConfigParameter p, char* v) {
  switch (p) {
    case CONF_DATA_FILE:
      v[strcspn(v, "\r\n")] = 0; // remove trailing newline character if it exists
      conf->dataFile = strdup(v);
      break;
    case CONF_PAGE_SIZE:
      conf->pageSize = atoi(v);
      break;
    case CONF_BUFPOOL_SIZE:
      conf->bufpoolSize = atoi(v);
  }
}

static FILE* read_config_file() {
  FILE* fp = fopen("burkeql.conf", "r");
  
  return fp;
}

static void close_config_file(FILE* fp) {
  fclose(fp);
}

/**
 * @brief Set the global config object
 * 
 * Reads the burkeql.conf file and sets the applicable conf properties
 * 
 * @param conf 
 * @return true 
 * @return false 
 */
bool set_global_config(Config* conf) {
  FILE* fp = read_config_file();
  char* line = NULL;
  size_t len = 0;
  ssize_t read;
  ConfigParameter p;

  if (fp == NULL) {
    printf("Unable to read config file: burkeql.conf\n");
    return false;
  }

  /**
   * loops through the config file and sets values in the
   * global Config object
   */
  while ((read = getline(&line, &len, fp)) != -1) {
    // skip comment lines or empty lines
    if (strncmp(line, "#", 1) == 0 || read <= 1) continue;

    // parse out the key and value
    char* param = strtok(line, "=");
    char* value = strtok(NULL, "=");

    p = parse_config_param(param);

    if (p == CONF_UNRECOGNIZED) continue;

    set_config_value(conf, p, value);
  }

  if (line) free(line);

  close_config_file(fp);

  return true;
}