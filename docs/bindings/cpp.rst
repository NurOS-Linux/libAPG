C++ bindings
============

The ``apg++`` headers in ``bindings/cpp/include/apg++/`` are header-only
C++17 wrappers over the C API. Include ``<apg++/apg.hpp>`` to get all of them.
They are installed with ``-Dcxx_bindings=enabled`` and link against the same
``libapg``.

Design
------

Owning classes (``apg::Database``, ``apg::Package``, ``apg::PackageMetadata``,
``apg::Transaction``) free their C object in the destructor. Calls that return
NULL for an object that must exist, such as ``Database::open()`` and
``Package::parse()``, throw ``apg::Error``, which derives from
``std::runtime_error``. Transaction calls return the C ``trans_error_t``
value and do not throw.

``apg::InstallPolicy`` exposes ``require_signature`` and ``keyring_dir``. The
``skip_dependency_check`` and ``skip_dependents_check`` flags of the C
``install_policy`` are not available through it, and stay false.

Example
-------

.. code-block:: cpp

   #include <apg++/apg.hpp>

   apg::Database db = apg::Database::open(db_path);
   apg::Package pkg = apg::Package::parse(archive_path, root);
   apg::Transaction trans(db);

   trans.add_install(pkg);
   trans_error_t err = trans.prepare();
   if (err == TRANS_OK)
       err = trans.commit(root);

Tests
-----

``-Dcxx_bindings=enabled`` builds and registers ``apgxx-test`` with
``meson test``.
