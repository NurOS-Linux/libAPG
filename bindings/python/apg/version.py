# SPDX-License-Identifier: GPL-3.0-only
# SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>

import ctypes
import enum

from . import _lib
from ._util import decode_ptr, take_str


class VerOp(enum.IntEnum):
    ANY = _lib.VER_OP_ANY
    EQ = _lib.VER_OP_EQ
    NEQ = _lib.VER_OP_NEQ
    LT = _lib.VER_OP_LT
    LE = _lib.VER_OP_LE
    GT = _lib.VER_OP_GT
    GE = _lib.VER_OP_GE


class DepConstraint:
    def __init__(self, name, op=VerOp.ANY, version=None):
        self.name = name
        self.op = VerOp(op)
        self.version = version

    def __repr__(self):
        return (
            f"DepConstraint(name={self.name!r}, op={self.op!r}, "
            f"version={self.version!r})"
        )

    def __eq__(self, other):
        if not isinstance(other, DepConstraint):
            return NotImplemented
        return (
            self.name == other.name
            and self.op == other.op
            and self.version == other.version
        )


def dep_constraint_parse(text):
    raw = _lib.dep_constraint_parse(text.encode())
    result = DepConstraint(
        decode_ptr(raw.name), raw.op, decode_ptr(raw.version)
    )
    _lib.dep_constraint_free(ctypes.byref(raw))
    return result


def dep_constraint_to_str(constraint):
    name_buf = ctypes.create_string_buffer(constraint.name.encode())
    version_buf = (
        ctypes.create_string_buffer(constraint.version.encode())
        if constraint.version
        else None
    )
    raw = _lib.DepConstraint(
        ctypes.addressof(name_buf),
        int(constraint.op),
        ctypes.addressof(version_buf) if version_buf else None,
    )
    return take_str(_lib.dep_constraint_to_str(ctypes.byref(raw)))


def ver_compare(a, b):
    return _lib.ver_compare(a.encode(), b.encode())


def ver_satisfies(pkg_version, op, constraint_version):
    return bool(
        _lib.ver_satisfies(
            pkg_version.encode(),
            int(op),
            constraint_version.encode() if constraint_version else None,
        )
    )
