// NurOS Ruzen42 2026 apg/config.h
// Last change: Feb 5

#pragma once

typedef enum {
  HTTP,
  FTP,
  RSYNC,
} repo_type;

typedef struct {
  const char *url;
  repo_type type;
} repo;

// all config in /etc/apg.conf
typedef struct {
  int db_size;
  char *tmp_dir;
  int repo_count;
  repo *repos;
} config;

config *parse_config(char *);
void set_config(config *);
void config_free(config *);

