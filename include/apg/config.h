// NurOS Ruzen42 2026 apg/config.h
// Last change: Jan 12
#ifndef APG_CONFIG_H
#define APG_CONFIG_H 


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
  size_t repo_count;
  repo *repos;
} config;

config *parse_config(char *);
void set_config(config *);
void config_free(config *);

#endif //APG_CONFIG_H
