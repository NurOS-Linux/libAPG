// NurOS Ruzen42 2025 apg/archive.h
// Last change: Jan 11
#ifndef APG_CONFIG_H
#define APG_CONFIG_H 

typedef struct {
  int db_size;
} config;

config parse_config(char *);
void set_config(config *);

#endif //APG_CONFIG_H
