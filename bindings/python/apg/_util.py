# SPDX-License-Identifier: GPL-3.0-only
# SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>

import ctypes

from . import _lib


def decode(s):
    if s is None:
        return None
    return s.decode()


def decode_ptr(addr):
    if not addr:
        return None
    return ctypes.cast(addr, ctypes.c_char_p).value.decode()


def take_str(addr):
    if not addr:
        return None
    value = ctypes.cast(addr, ctypes.c_char_p).value.decode()
    _lib.free(addr)
    return value


def take_str_array(addr, count):
    result = []
    for i in range(count):
        result.append(take_str(addr[i]))
    _lib.free(addr)
    return result


def borrowed_str_array(addr, count):
    if not addr:
        return []
    result = [addr[i].decode() for i in range(count)]
    _lib.free(addr)
    return result


def str_list_to_list(raw):
    return [raw.items[i].decode() for i in range(raw.count)]


def alloc_str_list(values):
    count = len(values)
    if count == 0:
        return _lib.StrList(None, 0)
    addr = _lib.c_malloc(count * ctypes.sizeof(ctypes.c_char_p))
    arr = ctypes.cast(addr, ctypes.POINTER(ctypes.c_char_p))
    for i, v in enumerate(values):
        arr[i] = _lib.c_strdup(v)
    return _lib.StrList(arr, count)


def alloc_dep_constraint_list(constraints):
    count = len(constraints)
    if count == 0:
        return _lib.DepConstraintList(None, 0)
    addr = _lib.c_malloc(count * ctypes.sizeof(_lib.DepConstraint))
    arr = ctypes.cast(addr, ctypes.POINTER(_lib.DepConstraint))
    for i, c in enumerate(constraints):
        arr[i].name = _lib.c_strdup(c.name)
        arr[i].op = int(c.op)
        arr[i].version = _lib.c_strdup(c.version) if c.version else None
    return _lib.DepConstraintList(arr, count)


def dep_constraint_list_to_list(raw):
    from .version import DepConstraint, VerOp

    result = []
    for i in range(raw.count):
        item = raw.items[i]
        result.append(
            DepConstraint(
                decode_ptr(item.name), VerOp(item.op), decode_ptr(item.version)
            )
        )
    return result
