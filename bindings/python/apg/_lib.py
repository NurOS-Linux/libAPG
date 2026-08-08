# SPDX-License-Identifier: GPL-3.0-only
# SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>

import ctypes
import ctypes.util
import os


def _load_library():
    override = os.environ.get("APG_LIBRARY_PATH")
    if override:
        return ctypes.CDLL(override)

    name = ctypes.util.find_library("apg")
    if name:
        return ctypes.CDLL(name)

    for candidate in ("libapg.so", "libapg.so.2", "libapg.dylib"):
        try:
            return ctypes.CDLL(candidate)
        except OSError:
            continue

    raise OSError(
        "could not locate libapg; set APG_LIBRARY_PATH to the .so path"
    )


lib = _load_library()

VER_OP_ANY = 0
VER_OP_EQ = 1
VER_OP_NEQ = 2
VER_OP_LT = 3
VER_OP_LE = 4
VER_OP_GT = 5
VER_OP_GE = 6

DEP_OK = 0
DEP_ERR_NOMEM = 1
DEP_ERR_CYCLE = 2
DEP_ERR_MISSING = 3
DEP_ERR_CONFLICT = 4
DEP_ERR_VERSION = 5

TRANS_OP_INSTALL = 0
TRANS_OP_REMOVE = 1
TRANS_OP_UPGRADE = 2

TRANS_OK = 0
TRANS_ERR_NOMEM = 1
TRANS_ERR_CONFLICT = 2
TRANS_ERR_MISSING_DEP = 3
TRANS_ERR_CYCLE = 4
TRANS_ERR_NOT_PREPARED = 5
TRANS_ERR_ALREADY_COMMITTED = 6
TRANS_ERR_INSTALL_FAILED = 7
TRANS_ERR_UNSIGNED = 8
TRANS_ERR_HAS_DEPENDENTS = 9
TRANS_ERR_FILE_CONFLICT = 10
TRANS_ERR_HELD = 11

JOURNAL_INSTALL = 0
JOURNAL_REMOVE = 1
JOURNAL_ROLLBACK = 2

JOURNAL_STATUS_OK = 0
JOURNAL_STATUS_FAILED = 1


class StrList(ctypes.Structure):
    _fields_ = [
        ("items", ctypes.POINTER(ctypes.c_char_p)),
        ("count", ctypes.c_int),
    ]


class DepConstraint(ctypes.Structure):
    _fields_ = [
        ("name", ctypes.c_void_p),
        ("op", ctypes.c_int),
        ("version", ctypes.c_void_p),
    ]


class DepConstraintList(ctypes.Structure):
    _fields_ = [
        ("items", ctypes.POINTER(DepConstraint)),
        ("count", ctypes.c_int),
    ]


class PackageMetadata(ctypes.Structure):
    _fields_ = [
        ("name", ctypes.c_void_p),
        ("version", ctypes.c_void_p),
        ("type", ctypes.c_void_p),
        ("architecture", ctypes.c_void_p),
        ("description", ctypes.c_void_p),
        ("maintainer", ctypes.c_void_p),
        ("license", ctypes.c_void_p),
        ("homepage", ctypes.c_void_p),
        ("tags", StrList),
        ("dependencies", DepConstraintList),
        ("conflicts", StrList),
        ("provides", StrList),
        ("replaces", StrList),
        ("conf", StrList),
    ]


class Package(ctypes.Structure):
    _fields_ = [
        ("meta", ctypes.POINTER(PackageMetadata)),
        ("pkg_path", ctypes.c_void_p),
        ("package_files", StrList),
        ("installed_by_hand", ctypes.c_bool),
        ("held", ctypes.c_bool),
    ]


class DbStats(ctypes.Structure):
    _fields_ = [
        ("package_count", ctypes.c_int),
        ("file_count", ctypes.c_int),
    ]


class InstallPolicy(ctypes.Structure):
    _fields_ = [
        ("require_signature", ctypes.c_bool),
        ("keyring_dir", ctypes.c_char_p),
    ]


PackagePtr = ctypes.POINTER(Package)
PackageMetadataPtr = ctypes.POINTER(PackageMetadata)

c_void_p = ctypes.c_void_p
c_char_p = ctypes.c_char_p
c_size_t = ctypes.c_size_t
c_int = ctypes.c_int
c_bool = ctypes.c_bool


def _sig(name, restype, argtypes):
    fn = getattr(lib, name)
    fn.restype = restype
    fn.argtypes = argtypes
    return fn


dep_constraint_parse = _sig("dep_constraint_parse", DepConstraint, [c_char_p])
dep_constraint_to_str = _sig(
    "dep_constraint_to_str", c_void_p, [ctypes.POINTER(DepConstraint)]
)
dep_constraint_free = _sig(
    "dep_constraint_free", None, [ctypes.POINTER(DepConstraint)]
)
dep_constraint_list_free = _sig(
    "dep_constraint_list_free", None, [ctypes.POINTER(DepConstraintList)]
)
ver_compare = _sig("ver_compare", c_int, [c_char_p, c_char_p])
ver_satisfies = _sig("ver_satisfies", c_bool, [c_char_p, c_int, c_char_p])

str_list_free = _sig("str_list_free", None, [ctypes.POINTER(StrList)])
package_metadata_free = _sig(
    "package_metadata_free", None, [PackageMetadataPtr]
)
package_free = _sig("package_free", None, [PackagePtr])
package_new = _sig("package_new", PackagePtr, [])
package_metadata_new = _sig("package_metadata_new", PackageMetadataPtr, [])
install_package = _sig("install_package", c_bool, [PackagePtr])
install_package_in_root = _sig(
    "install_package_in_root", c_bool, [PackagePtr, c_char_p]
)
package_collect_files = _sig(
    "package_collect_files", c_bool, [PackagePtr, c_char_p]
)
parse_package = _sig("parse_package", PackagePtr, [c_char_p, c_char_p])

package_to_json = _sig("package_to_json", c_void_p, [PackagePtr])
package_from_json = _sig("package_from_json", PackagePtr, [c_char_p, c_size_t])
package_metadata_from_file = _sig(
    "package_metadata_from_file", PackageMetadataPtr, [c_char_p]
)
package_metadata_from_json = _sig(
    "package_metadata_from_json", PackageMetadataPtr, [c_char_p, c_size_t]
)

db_open = _sig("db_open", c_void_p, [c_char_p])
db_open_readonly = _sig("db_open_readonly", c_void_p, [c_char_p])
db_close = _sig("db_close", None, [c_void_p])
db_add = _sig("db_add", c_bool, [c_void_p, PackagePtr])
db_remove = _sig("db_remove", c_bool, [c_void_p, c_char_p])
db_set_hold = _sig("db_set_hold", c_bool, [c_void_p, c_char_p, c_bool])
db_get = _sig("db_get", PackagePtr, [c_void_p, c_char_p])
db_list = _sig(
    "db_list", ctypes.POINTER(PackagePtr), [c_void_p, ctypes.POINTER(c_int)]
)
db_owner = _sig("db_owner", c_void_p, [c_void_p, c_char_p])
db_stats = _sig("db_stats", c_bool, [c_void_p, ctypes.POINTER(DbStats)])
db_get_orphans = _sig(
    "db_get_orphans",
    ctypes.POINTER(c_void_p),
    [c_void_p, ctypes.POINTER(c_int)],
)
db_search = _sig(
    "db_search",
    ctypes.POINTER(PackagePtr),
    [c_void_p, c_char_p, ctypes.POINTER(c_int)],
)
db_get_dependents = _sig(
    "db_get_dependents",
    ctypes.POINTER(c_void_p),
    [c_void_p, c_char_p, ctypes.POINTER(c_int)],
)
db_verify = _sig(
    "db_verify", c_void_p, [c_void_p, c_char_p, ctypes.POINTER(c_int)]
)
db_verify_free = _sig("db_verify_free", None, [c_void_p, c_int])
db_verify_issue_at = _sig(
    "db_verify_issue_at", c_void_p, [c_void_p, c_int, c_int]
)
db_verify_issue_pkg_name = _sig(
    "db_verify_issue_pkg_name", c_char_p, [c_void_p]
)
db_verify_issue_missing_count = _sig(
    "db_verify_issue_missing_count", c_int, [c_void_p]
)
db_verify_issue_missing_file_at = _sig(
    "db_verify_issue_missing_file_at", c_char_p, [c_void_p, c_int]
)

dep_graph_new = _sig("dep_graph_new", c_void_p, [])
dep_graph_free = _sig("dep_graph_free", None, [c_void_p])
dep_graph_add = _sig("dep_graph_add", c_int, [c_void_p, PackageMetadataPtr])
dep_graph_add_installed = _sig(
    "dep_graph_add_installed", c_int, [c_void_p, PackageMetadataPtr]
)
dep_graph_resolve = _sig(
    "dep_graph_resolve",
    c_int,
    [
        c_void_p,
        c_char_p,
        ctypes.POINTER(ctypes.POINTER(c_char_p)),
        ctypes.POINTER(c_size_t),
    ],
)
dep_graph_resolve_parallel = _sig(
    "dep_graph_resolve_parallel",
    c_int,
    [
        c_void_p,
        ctypes.POINTER(c_char_p),
        c_size_t,
        ctypes.POINTER(ctypes.POINTER(c_char_p)),
        ctypes.POINTER(c_size_t),
    ],
)
dep_graph_has_cycle = _sig("dep_graph_has_cycle", c_bool, [c_void_p])
dep_graph_find_breaks = _sig(
    "dep_graph_find_breaks",
    c_int,
    [
        c_void_p,
        c_char_p,
        ctypes.POINTER(c_char_p),
        c_size_t,
        ctypes.POINTER(ctypes.POINTER(c_char_p)),
        ctypes.POINTER(c_size_t),
    ],
)
dep_graph_export_dot = _sig("dep_graph_export_dot", c_void_p, [c_void_p])

trans_step_op = _sig("trans_step_op", c_int, [c_void_p])
trans_step_pkg_name = _sig("trans_step_pkg_name", c_char_p, [c_void_p])
trans_step_pkg_version = _sig("trans_step_pkg_version", c_char_p, [c_void_p])
trans_step_explicit = _sig("trans_step_explicit", c_bool, [c_void_p])

trans_conflict_pkg_name = _sig("trans_conflict_pkg_name", c_char_p, [c_void_p])
trans_conflict_conflicts_with = _sig(
    "trans_conflict_conflicts_with", c_char_p, [c_void_p]
)

trans_file_conflict_path = _sig(
    "trans_file_conflict_path", c_char_p, [c_void_p]
)
trans_file_conflict_requested_by = _sig(
    "trans_file_conflict_requested_by", c_char_p, [c_void_p]
)
trans_file_conflict_owned_by = _sig(
    "trans_file_conflict_owned_by", c_char_p, [c_void_p]
)

trans_held_pkg_name = _sig("trans_held_pkg_name", c_char_p, [c_void_p])
trans_held_pkg_op = _sig("trans_held_pkg_op", c_int, [c_void_p])

trans_blocked_remove_pkg_name = _sig(
    "trans_blocked_remove_pkg_name", c_char_p, [c_void_p]
)
trans_blocked_remove_dependent_count = _sig(
    "trans_blocked_remove_dependent_count", c_int, [c_void_p]
)
trans_blocked_remove_dependent_at = _sig(
    "trans_blocked_remove_dependent_at", c_char_p, [c_void_p, c_int]
)

trans_set_policy = _sig(
    "trans_set_policy", None, [c_void_p, ctypes.POINTER(InstallPolicy)]
)
trans_prefer_provider = _sig(
    "trans_prefer_provider", None, [c_void_p, c_char_p, c_char_p]
)

trans_new = _sig("trans_new", c_void_p, [c_void_p])
trans_free = _sig("trans_free", None, [c_void_p])
trans_add_install = _sig("trans_add_install", c_int, [c_void_p, PackagePtr])
trans_add_remove = _sig("trans_add_remove", c_int, [c_void_p, c_char_p])
trans_add_upgrade = _sig("trans_add_upgrade", c_int, [c_void_p, PackagePtr])
trans_prepare = _sig("trans_prepare", c_int, [c_void_p])

trans_plan_count = _sig("trans_plan_count", c_size_t, [c_void_p])
trans_plan_at = _sig("trans_plan_at", c_void_p, [c_void_p, c_size_t])
trans_conflict_count = _sig("trans_conflict_count", c_size_t, [c_void_p])
trans_conflict_at = _sig("trans_conflict_at", c_void_p, [c_void_p, c_size_t])
trans_blocked_remove_count = _sig(
    "trans_blocked_remove_count", c_size_t, [c_void_p]
)
trans_blocked_remove_at = _sig(
    "trans_blocked_remove_at", c_void_p, [c_void_p, c_size_t]
)
trans_file_conflict_count = _sig(
    "trans_file_conflict_count", c_size_t, [c_void_p]
)
trans_file_conflict_at = _sig(
    "trans_file_conflict_at", c_void_p, [c_void_p, c_size_t]
)
trans_held_pkg_count = _sig("trans_held_pkg_count", c_size_t, [c_void_p])
trans_held_pkg_at = _sig("trans_held_pkg_at", c_void_p, [c_void_p, c_size_t])
trans_commit = _sig("trans_commit", c_int, [c_void_p, c_char_p])

journal_entry_free = _sig("journal_entry_free", None, [c_void_p])
journal_free_all = _sig(
    "journal_free_all", None, [ctypes.POINTER(c_void_p), c_int]
)
journal_entry_op = _sig("journal_entry_op", c_int, [c_void_p])
journal_entry_pkg_name = _sig("journal_entry_pkg_name", c_char_p, [c_void_p])
journal_entry_pkg_version = _sig(
    "journal_entry_pkg_version", c_char_p, [c_void_p]
)
journal_entry_timestamp = _sig(
    "journal_entry_timestamp", ctypes.c_long, [c_void_p]
)
journal_entry_status = _sig("journal_entry_status", c_int, [c_void_p])
journal_entry_uid = _sig("journal_entry_uid", ctypes.c_uint, [c_void_p])
journal_entry_explicit = _sig("journal_entry_explicit", c_bool, [c_void_p])

audit_read_all = _sig(
    "audit_read_all",
    ctypes.POINTER(c_void_p),
    [c_void_p, ctypes.POINTER(c_int)],
)

keyring_load = _sig("keyring_load", c_void_p, [c_char_p])
keyring_free = _sig("keyring_free", None, [c_void_p])
keyring_verify = _sig(
    "keyring_verify", c_bool, [c_void_p, c_char_p, c_char_p]
)
keyring_add_key = _sig(
    "keyring_add_key", c_bool, [c_char_p, c_char_p, c_char_p, c_void_p]
)

sign_verify = _sig("sign_verify", c_bool, [c_char_p, c_char_p, c_bool])
sign_file = _sig("sign_file", c_bool, [c_char_p, c_char_p])

unarchive_package = _sig("unarchive_package", c_bool, [PackagePtr])
unarchive_package_in_root = _sig(
    "unarchive_package_in_root", c_bool, [PackagePtr, c_char_p]
)

install_data_dir = _sig("install_data_dir", c_bool, [c_char_p, c_char_p])
install_home_dir = _sig("install_home_dir", c_bool, [c_char_p])
rollback_install = _sig("rollback_install", None, [c_char_p, c_char_p])
copy_dir = _sig("copy_dir", c_bool, [c_char_p, c_char_p])

run_script = _sig("run_script", c_bool, [c_char_p, c_char_p, c_char_p])
scripts_store_path = _sig(
    "scripts_store_path", c_void_p, [c_char_p, c_char_p]
)
scripts_persist = _sig(
    "scripts_persist", c_bool, [c_char_p, c_char_p, c_char_p]
)
scripts_persist_remove = _sig(
    "scripts_persist_remove", None, [c_char_p, c_char_p]
)


_libc = ctypes.CDLL(None)
_libc.free.restype = None
_libc.free.argtypes = [c_void_p]
_libc.strdup.restype = c_void_p
_libc.strdup.argtypes = [c_char_p]
_libc.malloc.restype = c_void_p
_libc.malloc.argtypes = [c_size_t]


def free(ptr):
    if ptr:
        _libc.free(ptr)


def c_malloc(size):
    addr = _libc.malloc(size)
    if not addr:
        raise MemoryError("malloc failed")
    return addr


def c_strdup(s):
    if isinstance(s, str):
        s = s.encode()
    addr = _libc.strdup(s)
    if not addr:
        raise MemoryError("strdup failed")
    return addr
