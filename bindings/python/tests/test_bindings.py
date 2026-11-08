# SPDX-License-Identifier: GPL-3.0-only
# SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>

import ctypes
import ctypes.util

import pytest

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
    assert db.set_installed_by_hand("pathtest", False)
    assert db.get("pathtest").installed_by_hand is False

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


def test_verify_against_archive_errors(tmp_path):
    with apg.Database.open(str(tmp_path)) as db:
        with pytest.raises(apg.ApgError):
            db.verify_against_archive(
                "missing", str(tmp_path / "none.tar"), str(tmp_path)
            )
    assert apg.DbContentKind.MODE_CHANGED == 3


def build_archive(tmp_path):
    import tarfile

    meta = tmp_path / "metadata.json"
    meta.write_text('{"name": "arch", "version": "2.0"}')
    data = tmp_path / "data" / "usr" / "bin"
    data.mkdir(parents=True)
    (data / "tool").write_text("x")
    archive = tmp_path / "pkg.tar.gz"
    with tarfile.open(archive, "w:gz") as tar:
        tar.add(meta, arcname="metadata.json")
        tar.add(tmp_path / "data", arcname="data")
    return str(archive)


def test_archive_inspection(tmp_path):
    archive = build_archive(tmp_path)

    with apg.PackageMetadata.from_archive(archive) as meta:
        assert meta.name == "arch"
        assert meta.version == "2.0"
    assert apg.archive.list_files(archive) == ["/usr/bin/tool"]

    with pytest.raises(apg.ApgError):
        apg.PackageMetadata.from_archive(str(tmp_path / "missing.tar"))
    with pytest.raises(apg.ApgError):
        apg.archive.list_files(str(tmp_path / "missing.tar"))


def test_step_required_by(tmp_path):
    with apg.Database.open(str(tmp_path)) as db:
        app = make_pkg("app")
        app.metadata.set_dependencies([apg.dep_constraint_parse("lib")])
        lib = make_pkg("lib")
        with apg.Transaction(db) as trans:
            assert trans.add_install(app) == apg.TransError.OK
            assert trans.add_candidate(lib) == apg.TransError.OK
            assert trans.prepare() == apg.TransError.OK
            steps = {step.pkg_name: step for step in trans.plan}
            assert steps["app"].required_by is None
            assert steps["lib"].required_by == "app"


def test_db_check(tmp_path):
    with apg.Database.open(str(tmp_path)) as db:
        pkg = make_pkg("checked")
        assert db.add(pkg)
        assert db.check() == []

    kinds = apg.DbCheckKind
    assert kinds.OWNER_NOT_LISTED == 6


def test_run_script_strict(tmp_path):
    scripts = tmp_path / "scripts"
    scripts.mkdir()
    script = scripts / "post-install"
    script.write_text("#!/bin/sh\nexit 0\n")
    script.chmod(0o755)

    assert apg.scripts.run_script(str(tmp_path), "post-install", "/")
    strict = apg.scripts.run_script_strict(str(tmp_path), "post-install", "/")
    assert strict in (True, False)
    assert apg.scripts.run_script_strict(str(tmp_path), "pre-install", "/")

    with apg.Database.open(str(tmp_path)) as db:
        with apg.Transaction(db) as trans:
            trans.set_require_sandbox(True)
            assert trans.prepare() == apg.TransError.OK


def test_transaction_estimate_space(tmp_path):
    with apg.Database.open(str(tmp_path)) as db:
        with apg.Transaction(db) as trans:
            assert trans.estimate_space(str(tmp_path)) is None
            assert trans.prepare() == apg.TransError.OK
            needed, available = trans.estimate_space(str(tmp_path))
            assert needed == 0
            assert available > 0


def make_keypair():
    name = ctypes.util.find_library("sodium")
    if not name:
        pytest.skip("libsodium not found")
    sodium = ctypes.CDLL(name)
    public = ctypes.create_string_buffer(32)
    secret = ctypes.create_string_buffer(64)
    assert sodium.sodium_init() >= 0
    assert sodium.crypto_sign_keypair(public, secret) == 0
    return public.raw, secret.raw


def test_sign_with_explicit_keys(tmp_path):
    public, secret = make_keypair()
    other_public, _ = make_keypair()
    (tmp_path / "public.key").write_bytes(public)
    (tmp_path / "secret.key").write_bytes(secret)
    (tmp_path / "other.key").write_bytes(other_public)

    pkg = tmp_path / "pkg.bin"
    sig = tmp_path / "pkg.sig"
    pkg.write_bytes(b"payload")

    assert apg.sign.sign_with_key(
        str(pkg), str(sig), str(tmp_path / "secret.key")
    )
    assert apg.sign.verify_with_key(
        str(pkg), str(sig), str(tmp_path / "public.key")
    )
    assert not apg.sign.verify_with_key(
        str(pkg), str(sig), str(tmp_path / "other.key")
    )

    pkg.write_bytes(b"tampered")
    assert not apg.sign.verify_with_key(
        str(pkg), str(sig), str(tmp_path / "public.key")
    )
    assert not apg.sign.sign_with_key(
        str(pkg), str(sig), str(tmp_path / "missing.key")
    )


def test_transitive_dependents_and_orphans(tmp_path):
    with apg.Database.open(str(tmp_path)) as db:
        core = make_pkg("core")
        lib = make_pkg("lib")
        lib.metadata.set_dependencies([apg.dep_constraint_parse("core")])
        app = make_pkg("app")
        app.metadata.set_dependencies([apg.dep_constraint_parse("lib")])
        app.installed_by_hand = True
        for pkg in (core, lib, app):
            assert db.add(pkg)
        assert db.set_installed_by_hand("core", False)
        assert db.set_installed_by_hand("lib", False)

        assert sorted(db.get_dependents_transitive("core")) == ["app", "lib"]
        assert db.get_dependents("core") == ["lib"]
        assert db.get_orphans_transitive() == []

        assert db.remove("app")
        assert db.get_orphans() == ["lib"]
        assert sorted(db.get_orphans_transitive()) == ["core", "lib"]
