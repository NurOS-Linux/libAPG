// NurOS Ruzen42 2026 apg/json.h
// Last change: Jan 21
#pragma once

#ifndef APG_JSON_H

#include <yyjson/cJSON.h>
#include "package.h"

yy*package_to_json(struct package *);

char *json_to_string(cJSON *);

#endif //APG_JSON_H
