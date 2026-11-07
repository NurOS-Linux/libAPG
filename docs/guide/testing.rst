Testing
=======

Unit tests
----------

``meson test -C build`` runs two executables:

``apg-test``
   Tests for the public API: dependency resolution, conflicts, install
   policy, transactions, the database, the journal, archive extraction, JSON
   parsing, directory copy, and signing. Tests create temporary directories
   under ``/tmp``.

``sat-test``
   Tests for the dependency solver.

Each test prints ``PASS`` or ``SKIP`` with a reason, and a failed assertion
aborts the run.

Signing tests
-------------

``sign_file()`` and ``sign_verify()`` read fixed key files from the
``keys_dir`` build option. ``test_sign_file_and_verify_roundtrip`` writes its
own ``public.key`` and ``secret.key`` there and removes them afterwards. It
skips itself when the directory is not writable or when either file already
exists, so real keys are never overwritten. To run it, configure the build
with a ``keys_dir`` that the current user can write:

.. code-block:: sh

   meson setup build -Dkeys_dir=$PWD/build-keys
   mkdir build-keys
   meson test -C build

Sanitizers
----------

CI runs the suite with AddressSanitizer and UndefinedBehaviorSanitizer:

.. code-block:: sh

   meson setup build-san -Db_sanitize=address,undefined -Db_lundef=false
   meson test -C build-san --print-errorlogs

Fuzzing
-------

With ``-Dfuzzing=enabled`` and clang, the harnesses in ``fuzz/`` are built
with libFuzzer, AddressSanitizer and UndefinedBehaviorSanitizer:

.. list-table::
   :header-rows: 1

   * - Target
     - Input
   * - ``fuzz-package-json``
     - Package JSON parser
   * - ``fuzz-dep-constraint``
     - Dependency constraint parser
   * - ``fuzz-sat-model``, ``fuzz-sat-model-vars``
     - SAT model construction
   * - ``fuzz-candidate-set``
     - Candidate set construction
   * - ``fuzz-db``
     - Database operations
   * - ``fuzz-install-data-dir``
     - ``install_data_dir()``
   * - ``fuzz-transaction-prepare``
     - ``trans_prepare()``
   * - ``fuzz-unarchive``
     - ``parse_package()`` on arbitrary archive bytes. The run aborts if
       extraction creates anything outside its own directory.
   * - ``fuzz-keyring``
     - ``keyring_load()``, ``keyring_verify()`` and ``keyring_add_key()`` with
       keys, signature and package content taken from the input

.. code-block:: sh

   CC=clang meson setup build-fuzz -Dfuzzing=enabled
   meson compile -C build-fuzz fuzz/fuzz-package-json
   ./build-fuzz/fuzz/fuzz-package-json -max_total_time=60 fuzz/corpus/package-json

Seed corpora are in ``fuzz/corpus/``, one directory per target without the
``fuzz-`` prefix. The CI smoke run builds every target and runs each for 30
seconds.

Bindings tests
--------------

``-Dpython_bindings=enabled`` adds the pytest suite in ``bindings/python/``.
``-Dcxx_bindings=enabled`` builds and runs ``apgxx-test`` for the C++
bindings.

Static checks
-------------

``scripts/checkpatch.py`` runs clang-format, the SPDX header check, the
documentation coverage check on ``include/``, and clang-tidy.
