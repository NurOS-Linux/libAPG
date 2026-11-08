# SPDX-License-Identifier: GPL-3.0-only
# SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>

import ctypes
import enum

from . import _lib
from .error import ApgError
from .package import Package
from ._util import decode, take_str, take_str_array


class DbVerifyIssue:
    def __init__(self, pkg_name, missing_files):
        self.pkg_name = pkg_name
        self.missing_files = missing_files

    def __repr__(self):
        return (
            f"DbVerifyIssue(pkg_name={self.pkg_name!r}, "
            f"missing_files={self.missing_files!r})"
        )


class DbContentKind(enum.IntEnum):
    MISSING = 0
    MODIFIED = 1
    TYPE_CHANGED = 2
    MODE_CHANGED = 3


class DbContentIssue:
    def __init__(self, kind, path):
        self.kind = kind
        self.path = path

    def __repr__(self):
        return f"DbContentIssue(kind={self.kind.name}, path={self.path!r})"


class DbCheckKind(enum.IntEnum):
    CORRUPT_RECORD = 0
    NAME_MISMATCH = 1
    FILES_WITHOUT_PACKAGE = 2
    FILE_NOT_OWNED = 3
    FILE_OWNED_BY_OTHER = 4
    OWNER_WITHOUT_PACKAGE = 5
    OWNER_NOT_LISTED = 6


class DbCheckIssue:
    def __init__(self, kind, pkg_name, path, other):
        self.kind = kind
        self.pkg_name = pkg_name
        self.path = path
        self.other = other

    def __repr__(self):
        return (
            f"DbCheckIssue(kind={self.kind.name}, "
            f"pkg_name={self.pkg_name!r}, path={self.path!r}, "
            f"other={self.other!r})"
        )


class Database:
    def __init__(self, ptr):
        self._ptr = ptr

    @classmethod
    def open(cls, path):
        ptr = _lib.db_open(path.encode())
        if not ptr:
            raise ApgError(f"db_open failed for {path!r}")
        return cls(ptr)

    @classmethod
    def open_readonly(cls, path):
        ptr = _lib.db_open_readonly(path.encode())
        if not ptr:
            raise ApgError(f"db_open_readonly failed for {path!r}")
        return cls(ptr)

    @property
    def raw(self):
        return self._ptr

    def add(self, pkg):
        return bool(_lib.db_add(self._ptr, pkg.raw))

    def remove(self, pkg_name):
        return bool(_lib.db_remove(self._ptr, pkg_name.encode()))

    def set_hold(self, pkg_name, held):
        return bool(_lib.db_set_hold(self._ptr, pkg_name.encode(), held))

    def set_installed_by_hand(self, pkg_name, by_hand):
        return bool(
            _lib.db_set_installed_by_hand(self._ptr, pkg_name.encode(), by_hand)
        )

    def get(self, name):
        ptr = _lib.db_get(self._ptr, name.encode())
        if not ptr:
            return None
        return Package(ptr)

    def list(self):
        count = _lib.c_int(0)
        raw = _lib.db_list(self._ptr, ctypes.byref(count))
        result = []
        if raw:
            for i in range(count.value):
                addr = ctypes.cast(raw[i], ctypes.c_void_p).value
                result.append(Package(ctypes.cast(addr, _lib.PackagePtr)))
            _lib.free(raw)
        return result

    def owner(self, path):
        return take_str(_lib.db_owner(self._ptr, path.encode()))

    def stats(self):
        out = _lib.DbStats()
        if not _lib.db_stats(self._ptr, ctypes.byref(out)):
            raise ApgError("db_stats failed")
        return out.package_count, out.file_count

    def get_orphans(self):
        count = _lib.c_int(0)
        raw = _lib.db_get_orphans(self._ptr, ctypes.byref(count))
        if not raw:
            return []
        return take_str_array(raw, count.value)

    def get_orphans_transitive(self):
        count = _lib.c_int(0)
        raw = _lib.db_get_orphans_transitive(self._ptr, ctypes.byref(count))
        if not raw:
            return []
        return take_str_array(raw, count.value)

    def search(self, query):
        count = _lib.c_int(0)
        raw = _lib.db_search(
            self._ptr, query.encode(), ctypes.byref(count)
        )
        result = []
        if raw:
            for i in range(count.value):
                addr = ctypes.cast(raw[i], ctypes.c_void_p).value
                result.append(Package(ctypes.cast(addr, _lib.PackagePtr)))
            _lib.free(raw)
        return result

    def get_dependents(self, pkg_name):
        count = _lib.c_int(0)
        raw = _lib.db_get_dependents(
            self._ptr, pkg_name.encode(), ctypes.byref(count)
        )
        if not raw:
            return []
        return take_str_array(raw, count.value)

    def get_providers(self, name):
        count = _lib.c_int(0)
        raw = _lib.db_get_providers(
            self._ptr, name.encode(), ctypes.byref(count)
        )
        if not raw:
            return []
        return take_str_array(raw, count.value)

    def get_dependents_transitive(self, pkg_name):
        count = _lib.c_int(0)
        raw = _lib.db_get_dependents_transitive(
            self._ptr, pkg_name.encode(), ctypes.byref(count)
        )
        if not raw:
            return []
        return take_str_array(raw, count.value)

    def verify(self, root_path):
        count = _lib.c_int(0)
        issues = _lib.db_verify(
            self._ptr, root_path.encode(), ctypes.byref(count)
        )
        result = []
        for i in range(count.value):
            issue = _lib.db_verify_issue_at(issues, count.value, i)
            pkg_name = decode(_lib.db_verify_issue_pkg_name(issue))
            missing_count = _lib.db_verify_issue_missing_count(issue)
            missing = [
                decode(_lib.db_verify_issue_missing_file_at(issue, j))
                for j in range(missing_count)
            ]
            result.append(DbVerifyIssue(pkg_name, missing))
        _lib.db_verify_free(issues, count.value)
        return result

    def verify_against_archive(self, pkg_name, archive_path, root_path):
        issues = _lib.c_void_p()
        count = _lib.c_int(0)
        ok = _lib.db_verify_against_archive(
            self._ptr,
            pkg_name.encode(),
            archive_path.encode(),
            root_path.encode(),
            ctypes.byref(issues),
            ctypes.byref(count),
        )
        if not ok:
            raise ApgError(
                f"cannot compare {pkg_name!r} with {archive_path!r}"
            )
        result = []
        for i in range(count.value):
            issue = _lib.db_content_issue_at(issues, count.value, i)
            result.append(
                DbContentIssue(
                    DbContentKind(_lib.db_content_issue_kind(issue)),
                    decode(_lib.db_content_issue_path(issue)),
                )
            )
        _lib.db_content_free(issues, count.value)
        return result

    def check(self):
        count = _lib.c_int(0)
        issues = _lib.db_check(self._ptr, ctypes.byref(count))
        result = []
        for i in range(count.value):
            issue = _lib.db_check_issue_at(issues, count.value, i)
            result.append(
                DbCheckIssue(
                    DbCheckKind(_lib.db_check_issue_kind(issue)),
                    decode(_lib.db_check_issue_pkg_name(issue)),
                    decode(_lib.db_check_issue_path(issue)),
                    decode(_lib.db_check_issue_other(issue)),
                )
            )
        _lib.db_check_free(issues, count.value)
        return result

    def close(self):
        if self._ptr:
            _lib.db_close(self._ptr)
            self._ptr = None

    def __del__(self):
        self.close()

    def __enter__(self):
        return self

    def __exit__(self, *exc_info):
        self.close()
