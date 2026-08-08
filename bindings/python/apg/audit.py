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
