Python bindings
===============

The ``apg`` package in ``bindings/python/`` wraps the C library with
``ctypes``. It needs Python 3.8 or newer and a built ``libapg`` shared
library.

Finding the library
-------------------

The package loads the library in this order:

1. The path in the ``APG_LIBRARY_PATH`` environment variable.
2. The result of ``ctypes.util.find_library("apg")``.
3. ``libapg.so``, ``libapg.so.2`` and ``libapg.dylib`` from the default
   search path.

If none of them loads, importing ``apg`` raises ``OSError``.

Modules
-------

The top-level package exports ``Database``, ``Package``, ``PackageMetadata``,
``Transaction``, ``InstallPolicy``, ``DependencyGraph``, ``JournalEntry``,
``DepConstraint`` and the enums ``TransError``, ``TransOp``, ``VerOp``,
``JournalOp`` and ``JournalStatus``. The modules ``archive``, ``audit``,
``copy``, ``install``, ``keyring``, ``scripts`` and ``sign`` hold the
function-style parts of the C API.

``Database``, ``Transaction`` and the other owning classes release their C
object on ``close()``, on garbage collection and when used as context
managers.

Errors
------

Constructors and factory methods such as ``Database.open()`` and
``Package.parse()`` raise ``ApgError`` when the C call returns NULL. Boolean C
functions return ``bool``. ``Transaction.add_install()``, ``prepare()`` and
``commit()`` return a ``TransError`` and do not raise, so the caller checks
the value.

Example
-------

.. code-block:: python

   import apg

   with apg.Database.open(db_path) as db:
       pkg = apg.Package.parse(archive_path, root)
       with apg.Transaction(db) as trans:
           trans.add_install(pkg)
           err = trans.prepare()
           if err == apg.TransError.OK:
               err = trans.commit(root)
           if err != apg.TransError.OK:
               print(err.name)

Tests
-----

Configure with ``-Dpython_bindings=enabled`` and run ``meson test``. The suite
is in ``bindings/python/tests/`` and uses pytest.
