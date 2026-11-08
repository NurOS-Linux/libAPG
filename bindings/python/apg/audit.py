# SPDX-License-Identifier: GPL-3.0-only
# SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>

import ctypes

from . import _lib
from .journal import JournalEntry


def read_all(db):
    count = ctypes.c_int(0)
    raw = _lib.audit_read_all(db.raw, ctypes.byref(count))
    result = []
    if raw:
        for i in range(count.value):
            result.append(JournalEntry(raw[i]))
        _lib.free(raw)
    return result


def _take_entries(raw, count):
    result = []
    if raw:
        for i in range(count):
            result.append(JournalEntry(raw[i]))
        _lib.free(raw)
    return result


def read_since(db, since):
    count = ctypes.c_int(0)
    raw = _lib.audit_read_since(db.raw, since, ctypes.byref(count))
    return _take_entries(raw, count.value)


def read_for_package(db, pkg_name):
    count = ctypes.c_int(0)
    raw = _lib.audit_read_for_package(
        db.raw, pkg_name.encode(), ctypes.byref(count)
    )
    return _take_entries(raw, count.value)
