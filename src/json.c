// SPDX-License-Identifier: GPL-3.0-only
// SPDX-FileCopyrightText: 2026 Ruzen42
// SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>

#include <yyjson.h>
#include <stdlib.h>
#include <string.h>

#include "../include/apg/json.h"

static struct str_list
parse_str_array(yyjson_val *arr)
{
    struct str_list list = {0};
    if (!yyjson_is_arr(arr)) return list;

    size_t count = yyjson_arr_size(arr);
    if (count == 0) return list;

    list.items = malloc(count * sizeof(char *));
    if (!list.items) return list;

    size_t idx, max;
    yyjson_val *val;
    yyjson_arr_foreach(arr, idx, max, val) {
        list.items[idx] = yyjson_is_str(val) ? strdup(yyjson_get_str(val)) : NULL;
    }
    list.count = (int)count;
    return list;
}

static void
add_str_array(yyjson_mut_doc *doc, yyjson_mut_val *obj, const char *key, struct str_list *list)
{
    yyjson_mut_val *arr = yyjson_mut_arr(doc);
    for (int i = 0; i < list->count; i++) {
        if (list->items[i])
            yyjson_mut_arr_add_str(doc, arr, list->items[i]);
    }
    yyjson_mut_obj_add_val(doc, obj, key, arr);
}

struct package_metadata *
package_metadata_from_file(const char *path)
{
    yyjson_doc *doc = yyjson_read_file(path, 0, NULL, NULL);
    if (!doc) return NULL;

    yyjson_val *root = yyjson_doc_get_root(doc);
    if (!yyjson_is_obj(root)) {
        yyjson_doc_free(doc);
        return NULL;
    }

    struct package_metadata *meta = package_metadata_new();
    if (!meta) {
        yyjson_doc_free(doc);
        return NULL;
    }

    yyjson_val *v;

    v = yyjson_obj_get(root, "name");
    if (yyjson_is_str(v)) meta->name = strdup(yyjson_get_str(v));

    v = yyjson_obj_get(root, "version");
    if (yyjson_is_str(v)) meta->version = strdup(yyjson_get_str(v));

    v = yyjson_obj_get(root, "type");
    if (yyjson_is_str(v)) meta->type = strdup(yyjson_get_str(v));

    v = yyjson_obj_get(root, "architecture");
    if (yyjson_is_str(v)) meta->architecture = strdup(yyjson_get_str(v));

    v = yyjson_obj_get(root, "description");
    if (yyjson_is_str(v)) meta->description = strdup(yyjson_get_str(v));

    v = yyjson_obj_get(root, "maintainer");
    if (yyjson_is_str(v)) meta->maintainer = strdup(yyjson_get_str(v));

    v = yyjson_obj_get(root, "license");
    if (yyjson_is_str(v)) meta->license = strdup(yyjson_get_str(v));

    v = yyjson_obj_get(root, "homepage");
    if (yyjson_is_str(v)) meta->homepage = strdup(yyjson_get_str(v));

    meta->tags         = parse_str_array(yyjson_obj_get(root, "tags"));
    meta->dependencies = parse_str_array(yyjson_obj_get(root, "dependencies"));
    meta->conflicts    = parse_str_array(yyjson_obj_get(root, "conflicts"));
    meta->provides     = parse_str_array(yyjson_obj_get(root, "provides"));
    meta->replaces     = parse_str_array(yyjson_obj_get(root, "replaces"));
    meta->conf         = parse_str_array(yyjson_obj_get(root, "conf"));

    yyjson_doc_free(doc);
    return meta;
}

char *
package_to_json(struct package *pkg)
{
    yyjson_mut_doc *doc = yyjson_mut_doc_new(NULL);
    yyjson_mut_val *root = yyjson_mut_obj(doc);
    yyjson_mut_doc_set_root(doc, root);

    struct package_metadata *m = pkg->meta;

    yyjson_mut_obj_add_str(doc, root, "name",         m->name         ? m->name         : "");
    yyjson_mut_obj_add_str(doc, root, "version",      m->version      ? m->version      : "");
    yyjson_mut_obj_add_str(doc, root, "type",         m->type         ? m->type         : "misc");
    yyjson_mut_obj_add_str(doc, root, "architecture", m->architecture ? m->architecture : "");
    yyjson_mut_obj_add_str(doc, root, "description",  m->description  ? m->description  : "");
    yyjson_mut_obj_add_str(doc, root, "maintainer",   m->maintainer   ? m->maintainer   : "");
    yyjson_mut_obj_add_str(doc, root, "license",      m->license      ? m->license      : "");
    yyjson_mut_obj_add_str(doc, root, "homepage",     m->homepage     ? m->homepage     : "");

    add_str_array(doc, root, "tags",         &m->tags);
    add_str_array(doc, root, "dependencies", &m->dependencies);
    add_str_array(doc, root, "conflicts",    &m->conflicts);
    add_str_array(doc, root, "provides",     &m->provides);
    add_str_array(doc, root, "replaces",     &m->replaces);
    add_str_array(doc, root, "conf",         &m->conf);

    char *json = yyjson_mut_write(doc, 0, NULL);
    yyjson_mut_doc_free(doc);
    return json;
}
