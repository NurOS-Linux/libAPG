// SPDX-License-Identifier: GPL-3.0-only
// SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <apg/error.h>
#include <apg/json.h>
#include <apg/package.h>
#include <apg/version.h>

#include "fs_helpers.h"

static const char *full_json =
    "{\"name\":\"demo\",\"version\":\"1:2.3.4-5\",\"type\":\"library\","
    "\"architecture\":\"x86_64\",\"description\":\"Demo \\\"quoted\\\" pkg\","
    "\"maintainer\":\"Dev <dev@example.org>\",\"license\":\"GPL-3.0-only\","
    "\"homepage\":\"https://example.org\",\"tags\":[\"a\",\"b\"],"
    "\"dependencies\":[\"libx\",\"liby>=1.2\",\"libz==3.0\",\"old<2\"],"
    "\"conflicts\":[\"bad\"],\"provides\":[\"virt\",\"virt2\"],"
    "\"replaces\":[\"older\"],\"conf\":[\"etc/demo.conf\"],"
    "\"installed_by_hand\":true,\"held\":true,\"pkg_path\":\"/cache/d.apg\"}";

void
test_json_parses_all_fields(void)
{
    struct package *pkg = package_from_json(full_json, strlen(full_json));
    assert(pkg && pkg->meta);
    struct package_metadata *m = pkg->meta;

    assert(strcmp(m->name, "demo") == 0);
    assert(strcmp(m->version, "1:2.3.4-5") == 0);
    assert(strcmp(m->type, "library") == 0);
    assert(strcmp(m->architecture, "x86_64") == 0);
    assert(strcmp(m->description, "Demo \"quoted\" pkg") == 0);
    assert(strcmp(m->maintainer, "Dev <dev@example.org>") == 0);
    assert(strcmp(m->license, "GPL-3.0-only") == 0);
    assert(strcmp(m->homepage, "https://example.org") == 0);
    assert(m->tags.count == 2 && strcmp(m->tags.items[1], "b") == 0);
    assert(m->dependencies.count == 4);
    assert(strcmp(m->dependencies.items[0].name, "libx") == 0);
    assert(m->dependencies.items[0].op == VER_OP_ANY);
    assert(strcmp(m->dependencies.items[1].name, "liby") == 0);
    assert(m->dependencies.items[1].op == VER_OP_GE);
    assert(strcmp(m->dependencies.items[1].version, "1.2") == 0);
    assert(m->dependencies.items[2].op == VER_OP_EQ);
    assert(m->dependencies.items[3].op == VER_OP_LT);
    assert(m->conflicts.count == 1);
    assert(m->provides.count == 2);
    assert(m->replaces.count == 1);
    assert(m->conf.count == 1);
    assert(pkg->installed_by_hand && pkg->held);
    assert(strcmp(pkg->pkg_path, "/cache/d.apg") == 0);

    package_free(pkg);
    printf("test_json_parses_all_fields: PASS\n");
}

void
test_json_roundtrip_is_stable(void)
{
    struct package *pkg = package_from_json(full_json, strlen(full_json));
    assert(pkg);

    char *first = package_to_json(pkg);
    assert(first);
    struct package *again = package_from_json(first, strlen(first));
    assert(again);
    char *second = package_to_json(again);
    assert(second && strcmp(first, second) == 0);

    assert(strcmp(again->meta->name, "demo") == 0);
    assert(again->meta->dependencies.count == 4);
    assert(again->meta->dependencies.items[3].op == VER_OP_LT);
    assert(again->installed_by_hand && again->held);

    free(first);
    free(second);
    package_free(pkg);
    package_free(again);
    printf("test_json_roundtrip_is_stable: PASS\n");
}

void
test_json_rejects_malformed_input(void)
{
    assert(package_from_json("", 0) == NULL);
    assert(package_from_json("{", 1) == NULL);
    assert(package_from_json("[]", 2) == NULL);
    assert(package_from_json("\"str\"", 5) == NULL);
    assert(package_from_json("null", 4) == NULL);
    assert(package_from_json("{\"name\":", 8) == NULL);
    assert(package_from_json("{} trailing", 11) == NULL);

    assert(package_metadata_from_json("", 0) == NULL);
    assert(package_metadata_from_json("[1,2]", 5) == NULL);
    assert(package_metadata_from_json("{\"name\":\"x\"", 11) == NULL);

    printf("test_json_rejects_malformed_input: PASS\n");
}

void
test_json_respects_length_argument(void)
{
    const char *s = "{\"name\":\"abc\",\"version\":\"1\"}GARBAGE";
    size_t len = strlen("{\"name\":\"abc\",\"version\":\"1\"}");
    struct package_metadata *m = package_metadata_from_json(s, len);
    assert(m && strcmp(m->name, "abc") == 0);
    package_metadata_free(m);

    struct package *p = package_from_json(s, len);
    assert(p && strcmp(p->meta->version, "1") == 0);
    package_free(p);

    printf("test_json_respects_length_argument: PASS\n");
}

void
test_json_ignores_wrongly_typed_scalars(void)
{
    const char *s = "{\"name\":42,\"version\":null,\"description\":[\"x\"],"
                    "\"installed_by_hand\":\"yes\",\"held\":1,"
                    "\"pkg_path\":false}";
    struct package *pkg = package_from_json(s, strlen(s));
    assert(pkg && pkg->meta);
    assert(pkg->meta->name == NULL);
    assert(pkg->meta->version == NULL);
    assert(pkg->meta->description == NULL);
    assert(!pkg->installed_by_hand);
    assert(!pkg->held);
    assert(pkg->pkg_path == NULL);
    package_free(pkg);

    printf("test_json_ignores_wrongly_typed_scalars: PASS\n");
}

void
test_json_string_lists_never_contain_null_items(void)
{
    const char *s = "{\"name\":\"n\",\"version\":\"1\","
                    "\"tags\":[1,\"t\",null],\"provides\":[\"p\",{},true],"
                    "\"conflicts\":[false],\"replaces\":[[]],"
                    "\"conf\":[3.5,\"c\"]}";
    struct package_metadata *m = package_metadata_from_json(s, strlen(s));
    assert(m);

    struct str_list *lists[] = {&m->tags, &m->provides, &m->conflicts,
                                &m->replaces, &m->conf};
    for (size_t l = 0; l < sizeof(lists) / sizeof(lists[0]); l++)
        for (int i = 0; i < lists[l]->count; i++)
            assert(lists[l]->items[i] != NULL);

    package_metadata_free(m);
    printf("test_json_string_lists_never_contain_null_items: PASS\n");
}

void
test_json_dependencies_never_contain_null_names(void)
{
    const char *s = "{\"name\":\"n\",\"version\":\"1\","
                    "\"dependencies\":[7,\"ok>=1\",null,{}]}";
    struct package_metadata *m = package_metadata_from_json(s, strlen(s));
    assert(m);
    for (int i = 0; i < m->dependencies.count; i++)
        assert(m->dependencies.items[i].name != NULL);

    package_metadata_free(m);
    printf("test_json_dependencies_never_contain_null_names: PASS\n");
}

void
test_json_defaults_when_serializing_empty_package(void)
{
    struct package *pkg = package_new();
    assert(pkg);
    char *json = package_to_json(pkg);
    assert(json);

    struct package *back = package_from_json(json, strlen(json));
    assert(back && back->meta);
    assert(back->meta->type && strcmp(back->meta->type, "misc") == 0);
    assert(back->pkg_path == NULL);
    assert(back->meta->tags.count == 0);
    assert(back->meta->dependencies.count == 0);
    assert(!back->installed_by_hand && !back->held);

    free(json);
    package_free(pkg);
    package_free(back);
    printf("test_json_defaults_when_serializing_empty_package: PASS\n");
}

void
test_json_unicode_and_escapes_survive_roundtrip(void)
{
    struct package *pkg = package_new();
    assert(pkg);
    pkg->meta->name = strdup("pkg");
    pkg->meta->version = strdup("1");
    pkg->meta->description =
        strdup("tab\there \"q\" back\\slash \xd0\x9f\xd1\x80"
               "\xd0\xb8\xd0\xb2\xd0\xb5\xd1\x82 \n end");

    char *json = package_to_json(pkg);
    assert(json);
    struct package *back = package_from_json(json, strlen(json));
    assert(back);
    assert(strcmp(back->meta->description, pkg->meta->description) == 0);

    free(json);
    package_free(pkg);
    package_free(back);
    printf("test_json_unicode_and_escapes_survive_roundtrip: PASS\n");
}

void
test_json_metadata_from_file(void)
{
    char *dir = fs_mktmp("jsonfile");
    char *path = fs_join(dir, "metadata.json");
    fs_write(path, full_json);

    struct package_metadata *m = package_metadata_from_file(path);
    assert(m && strcmp(m->name, "demo") == 0);
    assert(m->dependencies.count == 4);
    package_metadata_free(m);

    char *missing = fs_join(dir, "absent.json");
    assert(package_metadata_from_file(missing) == NULL);

    fs_write(path, "[1,2,3]");
    assert(package_metadata_from_file(path) == NULL);

    fs_write(path, "{not json");
    assert(package_metadata_from_file(path) == NULL);

    free(missing);
    free(path);
    fs_rmtree(dir);
    free(dir);
    printf("test_json_metadata_from_file: PASS\n");
}

void
test_dep_constraint_string_roundtrip(void)
{
    const char *inputs[] = {"pkg",           "pkg >= 1.0", "pkg <= 2",
                            "pkg > 1:3.0-1", "pkg < 4",    "pkg == 1.2.3",
                            "pkg != 5"};
    for (size_t i = 0; i < sizeof(inputs) / sizeof(inputs[0]); i++)
    {
        struct dep_constraint c = dep_constraint_parse(inputs[i]);
        assert(c.name && strcmp(c.name, "pkg") == 0);
        char *s = dep_constraint_to_str(&c);
        assert(s && strcmp(s, inputs[i]) == 0);
        free(s);
        dep_constraint_free(&c);
    }
    printf("test_dep_constraint_string_roundtrip: PASS\n");
}
