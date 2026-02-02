// NurOS Ruzen42 2025 apg/db.h
// Last change: Dec 31
#pragma once

#ifndef APG_JSON_H

#include <cjson/cJSON.h>
#include "package.h"

cJSON *package_to_json(struct package *);

char *json_to_string(cJSON *);

#endif //APG_JSON_H