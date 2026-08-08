# SPDX-License-Identifier: GPL-3.0-only
# SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>

import enum

from . import _lib
from ._util import decode


class JournalOp(enum.IntEnum):
    INSTALL = _lib.JOURNAL_INSTALL
    REMOVE = _lib.JOURNAL_REMOVE
    ROLLBACK = _lib.JOURNAL_ROLLBACK


class JournalStatus(enum.IntEnum):
    OK = _lib.JOURNAL_STATUS_OK
    FAILED = _lib.JOURNAL_STATUS_FAILED


class JournalEntry:
    def __init__(self, ptr):
        self._ptr = ptr

    @property
    def op(self):
        return JournalOp(_lib.journal_entry_op(self._ptr))

    @property
    def pkg_name(self):
        return decode(_lib.journal_entry_pkg_name(self._ptr))

    @property
    def pkg_version(self):
        return decode(_lib.journal_entry_pkg_version(self._ptr))

    @property
    def timestamp(self):
        return _lib.journal_entry_timestamp(self._ptr)

    @property
    def status(self):
        return JournalStatus(_lib.journal_entry_status(self._ptr))

    @property
    def uid(self):
        return _lib.journal_entry_uid(self._ptr)

    @property
    def is_explicit(self):
        return bool(_lib.journal_entry_explicit(self._ptr))

    def close(self):
        if self._ptr:
            _lib.journal_entry_free(self._ptr)
            self._ptr = None

    def __del__(self):
        self.close()
