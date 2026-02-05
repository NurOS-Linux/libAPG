// NurOS Ruzen42 2026 apg/json.h
// Last change: Feb 2
#pragma once

#include <yyjson.h>
#include "package.h"

yyjson_val *package_to_json(struct package *);

struct package_metadata *package_metadata_from_file(const char *path);

char *json_to_string(yyjson_val *);
