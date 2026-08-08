# SPDX-License-Identifier: GPL-3.0-only
# SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>

import ctypes
import enum

from . import _lib
from .error import ApgError
from ._util import decode


class TransOp(enum.IntEnum):
    INSTALL = _lib.TRANS_OP_INSTALL
    REMOVE = _lib.TRANS_OP_REMOVE
    UPGRADE = _lib.TRANS_OP_UPGRADE


class TransError(enum.IntEnum):
    OK = _lib.TRANS_OK
    NOMEM = _lib.TRANS_ERR_NOMEM
    CONFLICT = _lib.TRANS_ERR_CONFLICT
    MISSING_DEP = _lib.TRANS_ERR_MISSING_DEP
    CYCLE = _lib.TRANS_ERR_CYCLE
    NOT_PREPARED = _lib.TRANS_ERR_NOT_PREPARED
    ALREADY_COMMITTED = _lib.TRANS_ERR_ALREADY_COMMITTED
    INSTALL_FAILED = _lib.TRANS_ERR_INSTALL_FAILED
    UNSIGNED = _lib.TRANS_ERR_UNSIGNED
    HAS_DEPENDENTS = _lib.TRANS_ERR_HAS_DEPENDENTS
    FILE_CONFLICT = _lib.TRANS_ERR_FILE_CONFLICT
    HELD = _lib.TRANS_ERR_HELD


class TransStepView:
    def __init__(self, ptr):
        self._ptr = ptr

    @property
    def op(self):
        return TransOp(_lib.trans_step_op(self._ptr))

    @property
    def pkg_name(self):
        return decode(_lib.trans_step_pkg_name(self._ptr))

    @property
    def pkg_version(self):
        return decode(_lib.trans_step_pkg_version(self._ptr))

    @property
    def is_explicit(self):
        return bool(_lib.trans_step_explicit(self._ptr))


class TransConflictView:
    def __init__(self, ptr):
        self._ptr = ptr

    @property
    def pkg_name(self):
        return decode(_lib.trans_conflict_pkg_name(self._ptr))

    @property
    def conflicts_with(self):
        return decode(_lib.trans_conflict_conflicts_with(self._ptr))


class TransFileConflictView:
    def __init__(self, ptr):
        self._ptr = ptr

    @property
    def path(self):
        return decode(_lib.trans_file_conflict_path(self._ptr))

    @property
    def requested_by(self):
        return decode(_lib.trans_file_conflict_requested_by(self._ptr))

    @property
    def owned_by(self):
        return decode(_lib.trans_file_conflict_owned_by(self._ptr))


class TransHeldPkgView:
    def __init__(self, ptr):
        self._ptr = ptr

    @property
    def pkg_name(self):
        return decode(_lib.trans_held_pkg_name(self._ptr))

    @property
    def op(self):
        return TransOp(_lib.trans_held_pkg_op(self._ptr))


class TransBlockedRemoveView:
    def __init__(self, ptr):
        self._ptr = ptr

    @property
    def pkg_name(self):
        return decode(_lib.trans_blocked_remove_pkg_name(self._ptr))

    @property
    def dependents(self):
        count = _lib.trans_blocked_remove_dependent_count(self._ptr)
        return [
            decode(_lib.trans_blocked_remove_dependent_at(self._ptr, i))
            for i in range(count)
        ]


class Transaction:
    def __init__(self, db):
        self._ptr = _lib.trans_new(db.raw)
        if not self._ptr:
            raise ApgError("trans_new failed")

    @property
    def raw(self):
        return self._ptr

    def set_policy(self, policy):
        raw = policy.to_c()
        _lib.trans_set_policy(self._ptr, ctypes.byref(raw))

    def clear_policy(self):
        _lib.trans_set_policy(self._ptr, None)

    def prefer_provider(self, name, pkg_name):
        _lib.trans_prefer_provider(
            self._ptr, name.encode(), pkg_name.encode()
        )

    def add_install(self, pkg):
        return TransError(_lib.trans_add_install(self._ptr, pkg.raw))

    def add_remove(self, pkg_name):
        return TransError(_lib.trans_add_remove(self._ptr, pkg_name.encode()))

    def add_upgrade(self, pkg):
        return TransError(_lib.trans_add_upgrade(self._ptr, pkg.raw))

    def prepare(self):
        return TransError(_lib.trans_prepare(self._ptr))

    @property
    def plan(self):
        count = _lib.trans_plan_count(self._ptr)
        return [
            TransStepView(_lib.trans_plan_at(self._ptr, i))
            for i in range(count)
        ]

    @property
    def conflicts(self):
        count = _lib.trans_conflict_count(self._ptr)
        return [
            TransConflictView(_lib.trans_conflict_at(self._ptr, i))
            for i in range(count)
        ]

    @property
    def blocked_removes(self):
        count = _lib.trans_blocked_remove_count(self._ptr)
        return [
            TransBlockedRemoveView(_lib.trans_blocked_remove_at(self._ptr, i))
            for i in range(count)
        ]

    @property
    def file_conflicts(self):
        count = _lib.trans_file_conflict_count(self._ptr)
        return [
            TransFileConflictView(_lib.trans_file_conflict_at(self._ptr, i))
            for i in range(count)
        ]

    @property
    def held_pkgs(self):
        count = _lib.trans_held_pkg_count(self._ptr)
        return [
            TransHeldPkgView(_lib.trans_held_pkg_at(self._ptr, i))
            for i in range(count)
        ]

    def commit(self, root_path):
        return TransError(_lib.trans_commit(self._ptr, root_path.encode()))

    def close(self):
        if self._ptr:
            _lib.trans_free(self._ptr)
            self._ptr = None

    def __del__(self):
        self.close()

    def __enter__(self):
        return self

    def __exit__(self, *exc_info):
        self.close()
