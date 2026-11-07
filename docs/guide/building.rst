Building
========

Dependencies
------------

.. list-table::
   :header-rows: 1

   * - Dependency
     - Use
   * - Meson, Ninja, pkg-config
     - Build tools
   * - libarchive
     - Package archive extraction
   * - lmdb
     - Package database
   * - yyjson
     - JSON parsing and serialization
   * - libsodium
     - Package signing and the keyring
   * - libseccomp (optional)
     - Detected at configure time. The current sources do not use it.

Meson falls back to the wrap files in ``subprojects/`` for libarchive, lmdb
and yyjson when the system packages are not found.

Configure and build
-------------------

.. code-block:: sh

   meson setup build
   meson compile -C build
   meson test -C build --print-errorlogs

The test executables are not built by default. ``meson test`` builds them.

With Nix:

.. code-block:: sh

   nix build

Options
-------

Options are set with ``-Dname=value`` at ``meson setup`` or with
``meson configure``.

.. list-table::
   :header-rows: 1

   * - Option
     - Default
     - Meaning
   * - ``keyring_dir``
     - ``/etc/apg/trusted.d``
     - Directory of trusted public keys used when an install policy gives no
       keyring directory
   * - ``keys_dir``
     - ``/etc/apg/keys``
     - Directory holding ``public.key`` and ``secret.key`` for
       ``sign_file()`` and ``sign_verify()``
   * - ``tmp_dir``
     - ``/tmp/apg``
     - Temporary directory, resolved inside the target root, used during
       package extraction and upgrade backups
   * - ``db_mapsize``
     - ``10485760``
     - Initial LMDB map size in bytes
   * - ``fuzzing``
     - ``disabled``
     - Build the libFuzzer harnesses in ``fuzz/``. Needs clang.
   * - ``cxx_bindings``
     - ``disabled``
     - Install the header-only C++ bindings. Needs a C++17 compiler.
   * - ``python_bindings``
     - ``disabled``
     - Run the Python bindings test suite. Needs python3 and pytest.

On FreeBSD the default ``keyring_dir`` and ``keys_dir`` change to
``/usr/local/etc/apg/trusted.d`` and ``/usr/local/etc/apg/keys`` unless
overridden.

Cross compilation
-----------------

Meson cross files for aarch64, arm, i386, mips, mips64, ppc32, ppc32le, ppc64,
ppc64le, riscv32, riscv64 and x86_64 are in ``cross/``:

.. code-block:: sh

   meson setup build-aarch64 --cross-file cross/cross-aarch64.txt

Sanitizers
----------

.. code-block:: sh

   meson setup build-san -Db_sanitize=address,undefined -Db_lundef=false
   meson test -C build-san --print-errorlogs
