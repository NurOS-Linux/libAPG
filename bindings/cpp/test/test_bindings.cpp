// SPDX-License-Identifier: GPL-3.0-only
// SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>

#include <apg++/apg.hpp>

#include <unistd.h>

#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

namespace
{

std::string
make_tmp_dir(const char *prefix)
{
    std::string tmpl = std::string("/tmp/") + prefix + "-XXXXXX";
    std::vector<char> buf(tmpl.begin(), tmpl.end());
    buf.push_back('\0');
    if (!mkdtemp(buf.data()))
        throw apg::Error("mkdtemp failed");
    return std::string(buf.data());
}

void
remove_tmp_db(const std::string &path)
{
    unlink((path + "/data.mdb").c_str());
    unlink((path + "/lock.mdb").c_str());
    rmdir(path.c_str());
}

apg::Package
make_simple_package(const std::string &name, const std::string &version = "1.0")
{
    apg::Package pkg = apg::Package::create();
    struct package *raw = pkg.get();
    raw->meta->name = strdup(name.c_str());
    raw->meta->version = strdup(version.c_str());
    raw->pkg_path = strdup("/nonexistent/test.pkg");
    raw->installed_by_hand = true;
    return pkg;
}

void
test_version()
{
    assert(apg::ver_compare("1.0", "2.0") < 0);
    assert(apg::ver_satisfies("2.0", VER_OP_GE, "1.0"));

    apg::DepConstraint c = apg::dep_constraint_parse("libfoo >= 2.0");
    assert(c.name == "libfoo");
    assert(c.op == VER_OP_GE);
    assert(c.version == "2.0");
    assert(apg::dep_constraint_to_str(c) == "libfoo >= 2.0");

    printf("test_version: PASS\n");
}

void
test_database_roundtrip()
{
    std::string db_path = make_tmp_dir("apgxx-db");
    apg::Database db = apg::Database::open(db_path);

    apg::Package pkg = make_simple_package("cppxx-pkg");
    assert(db.add(pkg));

    auto fetched = db.get("cppxx-pkg");
    assert(fetched.has_value());
    assert(fetched->metadata().name() == "cppxx-pkg");
    assert(fetched->metadata().version() == "1.0");

    assert(db.set_hold("cppxx-pkg", true));
    assert(db.set_installed_by_hand("cppxx-pkg", false));

    assert(db.check().empty());
    bool threw = false;
    try
    {
        db.verify_against_archive("cppxx-pkg", "/nonexistent/pkg.tar", "/");
    }
    catch (const apg::Error &)
    {
        threw = true;
    }
    assert(threw);

    apg::DbStats stats = db.stats();
    assert(stats.package_count >= 1);

    assert(db.remove("cppxx-pkg"));
    assert(!db.get("cppxx-pkg").has_value());

    remove_tmp_db(db_path);
    printf("test_database_roundtrip: PASS\n");
}

void
test_database_transitive_queries()
{
    std::string db_path = make_tmp_dir("apgxx-trans-q");
    apg::Database db = apg::Database::open(db_path);

    apg::Package core = make_simple_package("core");
    apg::Package lib = make_simple_package("lib");
    lib.get()->meta->dependencies.count = 1;
    lib.get()->meta->dependencies.items = static_cast<struct dep_constraint *>(
        malloc(sizeof(struct dep_constraint)));
    lib.get()->meta->dependencies.items[0] = dep_constraint_parse("core");
    lib.get()->installed_by_hand = false;
    core.get()->installed_by_hand = false;
    assert(db.add(core));
    assert(db.add(lib));

    auto dependents = db.get_dependents_transitive("core");
    assert(dependents.size() == 1 && dependents[0] == "lib");

    auto orphans = db.get_orphans_transitive();
    assert(orphans.size() == 2);

    remove_tmp_db(db_path);
    printf("test_database_transitive_queries: PASS\n");
}

void
test_sign_with_explicit_keys()
{
    std::string dir = make_tmp_dir("apgxx-sign");
    std::string pkg = dir + "/pkg.bin";
    std::string sig = dir + "/pkg.sig";
    FILE *f = fopen(pkg.c_str(), "wb");
    assert(f);
    fputs("payload", f);
    fclose(f);

    assert(!apg::sign_file_with_key(pkg, sig, dir + "/missing.key"));
    assert(!apg::sign_verify_with_key(pkg, sig, dir + "/missing.key"));

    unlink(pkg.c_str());
    remove_tmp_db(dir);
    printf("test_sign_with_explicit_keys: PASS\n");
}

void
test_scripts_strict_and_sandbox_flag()
{
    assert(apg::run_script_strict("/nonexistent-apg-dir", "post-install", "/"));

    std::string db_path = make_tmp_dir("apgxx-sandbox");
    apg::Database db = apg::Database::open(db_path);
    apg::Transaction trans(db);
    trans.set_require_sandbox(true);
    assert(!trans.estimate_space(db_path).has_value());
    assert(trans.prepare() == TRANS_OK);
    auto space = trans.estimate_space(db_path);
    assert(space.has_value() && space->first == 0 && space->second > 0);
    remove_tmp_db(db_path);
    printf("test_scripts_strict_and_sandbox_flag: PASS\n");
}

void
test_dependency_graph()
{
    apg::PackageMetadata base = apg::PackageMetadata::create();
    base.get()->name = strdup("base");
    base.get()->version = strdup("1.0");

    apg::PackageMetadata app = apg::PackageMetadata::create();
    app.get()->name = strdup("app");
    app.get()->version = strdup("1.0");
    app.get()->dependencies.count = 1;
    app.get()->dependencies.items = static_cast<struct dep_constraint *>(
        malloc(sizeof(struct dep_constraint)));
    app.get()->dependencies.items[0] = dep_constraint_parse("base");

    apg::DependencyGraph graph;
    assert(graph.add(base) == DEP_OK);
    assert(graph.add(app) == DEP_OK);

    std::vector<std::string> order;
    assert(graph.resolve("app", order) == DEP_OK);
    assert(order.size() == 2);
    assert(order[0] == "base");
    assert(order[1] == "app");

    printf("test_dependency_graph: PASS\n");
}

void
test_transaction_policy()
{
    std::string db_path = make_tmp_dir("apgxx-trans-db");
    apg::Database db = apg::Database::open(db_path);

    apg::Package pkg = make_simple_package("trans-pkg");
    apg::Transaction trans(db);
    assert(trans.add_install(pkg) == TRANS_OK);
    assert(trans.prepare() == TRANS_OK);

    apg::InstallPolicy policy;
    policy.require_signature = true;
    policy.keyring_dir = "/nonexistent/keys";
    trans.set_policy(policy);

    assert(trans.commit("/tmp") == TRANS_ERR_UNSIGNED);

    remove_tmp_db(db_path);
    printf("test_transaction_policy: PASS\n");
}

} // namespace

int
main()
{
    test_version();
    test_database_roundtrip();
    test_database_transitive_queries();
    test_sign_with_explicit_keys();
    test_scripts_strict_and_sandbox_flag();
    test_dependency_graph();
    test_transaction_policy();
    printf("All C++ binding tests passed.\n");
    return 0;
}
