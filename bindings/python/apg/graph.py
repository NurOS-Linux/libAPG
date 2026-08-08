# SPDX-License-Identifier: GPL-3.0-only
# SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>

import ctypes

from . import _lib
from .error import ApgError
from ._util import borrowed_str_array, take_str


class DependencyGraph:
    def __init__(self):
        self._ptr = _lib.dep_graph_new()
        if not self._ptr:
            raise ApgError("dep_graph_new failed")

    def add(self, meta):
        return _lib.dep_graph_add(self._ptr, meta.raw)

    def add_installed(self, meta):
        return _lib.dep_graph_add_installed(self._ptr, meta.raw)

    def has_cycle(self):
        return bool(_lib.dep_graph_has_cycle(self._ptr))

    def resolve(self, pkg_name):
        order = ctypes.POINTER(ctypes.c_char_p)()
        count = _lib.c_size_t(0)
        err = _lib.dep_graph_resolve(
            self._ptr,
            pkg_name.encode(),
            ctypes.byref(order),
            ctypes.byref(count),
        )
        return err, borrowed_str_array(order, count.value)

    def resolve_parallel(self, pkg_names):
        c_names = (ctypes.c_char_p * len(pkg_names))(
            *[n.encode() for n in pkg_names]
        )
        order = ctypes.POINTER(ctypes.c_char_p)()
        count = _lib.c_size_t(0)
        err = _lib.dep_graph_resolve_parallel(
            self._ptr,
            c_names,
            len(pkg_names),
            ctypes.byref(order),
            ctypes.byref(count),
        )
        return err, borrowed_str_array(order, count.value)

    def find_breaks(self, pkg_name, installed):
        c_installed = (ctypes.c_char_p * len(installed))(
            *[n.encode() for n in installed]
        )
        breaks = ctypes.POINTER(ctypes.c_char_p)()
        count = _lib.c_size_t(0)
        err = _lib.dep_graph_find_breaks(
            self._ptr,
            pkg_name.encode(),
            c_installed,
            len(installed),
            ctypes.byref(breaks),
            ctypes.byref(count),
        )
        return err, borrowed_str_array(breaks, count.value)

    def export_dot(self):
        return take_str(_lib.dep_graph_export_dot(self._ptr))

    def close(self):
        if self._ptr:
            _lib.dep_graph_free(self._ptr)
            self._ptr = None

    def __del__(self):
        self.close()

    def __enter__(self):
        return self

    def __exit__(self, *exc_info):
        self.close()
