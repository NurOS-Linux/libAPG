# SPDX-License-Identifier: GPL-3.0-only
# SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>

from . import _lib
from .error import ApgError


class Keyring:
    def __init__(self, ptr):
        self._ptr = ptr

    @classmethod
    def load(cls, keyring_dir):
        ptr = _lib.keyring_load(keyring_dir.encode())
        if not ptr:
            raise ApgError(f"keyring_load failed for {keyring_dir!r}")
        return cls(ptr)

    @property
    def raw(self):
        return self._ptr

    def verify(self, pkg_path, sig_path):
        return bool(
            _lib.keyring_verify(
                self._ptr, pkg_path.encode(), sig_path.encode()
            )
        )

    @staticmethod
    def add_key(keyring_dir, new_key_path, key_sig_path, trusted):
        return bool(
            _lib.keyring_add_key(
                keyring_dir.encode(),
                new_key_path.encode(),
                key_sig_path.encode(),
                trusted.raw,
            )
        )

    def close(self):
        if self._ptr:
            _lib.keyring_free(self._ptr)
            self._ptr = None

    def __del__(self):
        self.close()

    def __enter__(self):
        return self

    def __exit__(self, *exc_info):
        self.close()
