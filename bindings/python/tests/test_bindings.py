# SPDX-License-Identifier: GPL-3.0-only
# SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>

import apg


def make_pkg(name, version="1.0", provides=None):
    pkg = apg.Package.create()
    pkg.metadata.set_name(name)
    pkg.metadata.set_version(version)
    pkg.path = "/nonexistent/test.pkg"
    if provides:
        pkg.metadata.set_provides([provides])
    return pkg


def make_meta(name, version="1.0", provides=None):
    meta = apg.PackageMetadata.create()
    meta.set_name(name)
    meta.set_version(version)
    if provides:
        meta.set_provides([provides])
    return meta


def test_version():
    assert apg.ver_compare("1.0", "2.0") < 0
    assert apg.ver_satisfies("2.0", apg.VerOp.GE, "1.0")

    c = apg.dep_constraint_parse("libfoo >= 2.0")
    assert c.name == "libfoo"
    assert c.op == apg.VerOp.GE
    assert c.version == "2.0"
    assert apg.dep_constraint_to_str(c) == "libfoo >= 2.0"


def test_database_roundtrip(tmp_path):
    db = apg.Database.open(str(tmp_path))

    pkg = make_pkg("pathtest")
    pkg.path = "/var/cache/apg/pathtest-1.0.apg"
    assert db.add(pkg)

    fetched = db.get("pathtest")
    assert fetched is not None
    assert fetched.metadata.name == "pathtest"
    assert fetched.metadata.version == "1.0"
    assert fetched.path == "/var/cache/apg/pathtest-1.0.apg"

    assert db.set_hold("pathtest", True)

    pkg_count, _ = db.stats()
    assert pkg_count >= 1

    assert db.remove("pathtest")
    assert db.get("pathtest") is None

    fetched.close()
    pkg.close()
    db.close()


def test_db_list_and_search(tmp_path):
    db = apg.Database.open(str(tmp_path))

    pkg = make_pkg("searchable-pkg")
    pkg.metadata.set_description("a package used for search tests")
    assert db.add(pkg)

    names = [p.metadata.name for p in db.list()]
    assert "searchable-pkg" in names

    results = db.search("searchable")
    assert any(p.metadata.name == "searchable-pkg" for p in results)

    pkg.close()
    db.close()


def test_dependency_graph_prefers_installed():
    nginx = make_meta("nginx", "1.0", "webserver")
    apache = make_meta("apache", "1.0", "webserver")
    app = make_meta("app")
    app.set_dependencies([apg.dep_constraint_parse("webserver")])

    g = apg.DependencyGraph()
    g.add(apache)
    g.add(app)
    g.add_installed(nginx)

    err, order = g.resolve("app")
    assert err == 0
    assert order == ["nginx", "app"]

    dot = g.export_dot()
    assert "digraph libapg_deps" in dot
    assert '"nginx" [style=filled, fillcolor="lightgreen"]' in dot

    g.close()
    nginx.close()
    apache.close()
    app.close()


def test_dependency_graph_falls_back_without_installed():
    first = make_meta("apache", "1.0", "webserver")
    second = make_meta("caddy", "1.0", "webserver")
    app = make_meta("app")
    app.set_dependencies([apg.dep_constraint_parse("webserver")])

    g = apg.DependencyGraph()
    g.add(first)
    g.add(second)
    g.add(app)

    err, order = g.resolve("app")
    assert err == 0
    assert order == ["apache", "app"]

    g.close()
    first.close()
    second.close()
    app.close()


def test_transaction_policy(tmp_path):
    db = apg.Database.open(str(tmp_path))
    pkg = make_pkg("trans-pkg")

    trans = apg.Transaction(db)
    assert trans.add_install(pkg) == apg.TransError.OK
    assert trans.prepare() == apg.TransError.OK

    policy = apg.InstallPolicy(
        require_signature=True, keyring_dir="/nonexistent/keys"
    )
    trans.set_policy(policy)

    assert trans.commit("/tmp") == apg.TransError.UNSIGNED

    trans.close()
    pkg.close()
    db.close()


def test_transaction_prefer_provider_overrides_default(tmp_path):
    db = apg.Database.open(str(tmp_path))

    nginx_installed = make_pkg("nginx", "1.0", "webserver")
    assert db.add(nginx_installed)

    caddy = make_pkg("caddy", "2.0", "webserver")
    app = make_pkg("app")
    app.metadata.set_dependencies(
        [apg.dep_constraint_parse("webserver >= 2.0")]
    )

    trans = apg.Transaction(db)
    assert trans.add_install(caddy) == apg.TransError.OK
    assert trans.add_install(app) == apg.TransError.OK
    trans.prefer_provider("webserver", "caddy")

    assert trans.prepare() == apg.TransError.OK
    plan = trans.plan
    assert [step.pkg_name for step in plan] == ["caddy", "app"]

    trans.close()
    caddy.close()
    app.close()
    nginx_installed.close()
    db.close()


def test_transaction_default_prefers_installed_provider(tmp_path):
    db = apg.Database.open(str(tmp_path))

    nginx_installed = make_pkg("nginx", "1.0", "webserver")
    assert db.add(nginx_installed)

    caddy = make_pkg("caddy", "2.0", "webserver")
    app = make_pkg("app")
    app.metadata.set_dependencies(
        [apg.dep_constraint_parse("webserver >= 2.0")]
    )

    trans = apg.Transaction(db)
    assert trans.add_install(caddy) == apg.TransError.OK
    assert trans.add_install(app) == apg.TransError.OK

    assert trans.prepare() != apg.TransError.OK

    trans.close()
    caddy.close()
    app.close()
    nginx_installed.close()
    db.close()


def test_audit_log(tmp_path):
    db = apg.Database.open(str(tmp_path))
    pkg = make_pkg("audit-pkg")

    trans = apg.Transaction(db)
    trans.add_install(pkg)
    trans.prepare()
    trans.set_policy(apg.InstallPolicy(require_signature=False))
    trans.commit("/tmp")

    entries = apg.audit.read_all(db)
    assert len(entries) >= 1
    last = entries[-1]
    assert last.op == apg.JournalOp.INSTALL
    assert last.pkg_name == "audit-pkg"

    for e in entries:
        e.close()
    trans.close()
    pkg.close()
    db.close()
