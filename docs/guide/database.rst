Package database
================

The package database is an LMDB environment stored in a directory. It records
installed packages, the files each package owns, and the operation journal.

Opening
-------

``db_open()`` opens the environment for reading and writing and takes an
exclusive, non-blocking ``flock`` on ``db.lock`` inside the database
directory. If another process holds the lock, ``db_open()`` returns NULL and
``apg_last_error()`` reports that the database is locked.

``db_open_readonly()`` opens the environment read-only and does not take the
lock. Write functions on a read-only handle fail.

Writes made through one handle are serialized by a mutex in the handle.

Contents
--------

.. list-table::
   :header-rows: 1

   * - Database
     - Key
     - Value
   * - main (unnamed)
     - Package name
     - JSON record: metadata, ``installed_by_hand``, ``held``, ``pkg_path``
   * - ``files``
     - Package name
     - Newline-terminated list of the package's file paths
   * - ``file_owner``
     - File path
     - Name of the package that owns the path
   * - ``journal``
     - 8 byte big-endian timestamp, 4 byte sequence number
     - ``op|name|version|status|uid|explicit`` text record. ``|`` and ``%``
       inside name and version are written as ``%7C`` and ``%25``.

The ``journal`` database is created by the first journal write. When
``db_add()`` and ``db_remove()`` succeed, and when ``trans_commit()`` records an
install, upgrade or removal, the journal entry is written in the same LMDB
transaction as the package record, so the two are stored together or not at
all. A failed operation is journaled separately as ``failed``. If the journal
entry itself cannot be written, the operation fails. The main
database also holds the entries of the named databases, so it is not a plain
list of packages. Use ``db_list()`` and ``db_stats()`` instead of reading it
directly.

File paths recorded from a package archive start with ``/`` and are relative
to the install root.

Map size
--------

The initial map size is the ``db_mapsize`` build option (10 MiB by default).
When a write fails with ``MDB_MAP_FULL``, ``db_add()`` doubles the map size
and retries, up to eight times. Resizing requires that no other thread of the
process holds a read transaction on the same database at that moment. Other
processes pick up the larger size the next time they open the database.

Queries
-------

``db_get_dependents()`` lists installed packages that depend on a name,
including dependencies that name a ``provides`` entry of the package.

``db_get_orphans()`` lists packages that are not marked ``installed_by_hand``
and that no installed package depends on, directly or through ``provides``.
The check is not transitive: a chain of automatically installed packages
reports only its top.

``db_get_dependents_transitive()`` and ``db_get_orphans_transitive()`` follow
dependency chains. The first lists every package that depends on a name
directly or through other packages. The second treats packages marked
``installed_by_hand`` as roots and returns everything not reachable from them,
including chains and cycles of automatically installed packages. The held flag
is ignored, as in ``db_get_orphans()``. The result of each contains everything
the direct variant returns.

``db_search()`` matches a case-insensitive substring against the package name
and description.

``db_check()`` checks the database against itself, without touching the file
system. It reads the package records, the file lists and the ownership entries
in one snapshot and reports each inconsistency as an issue with a kind, the
package, and where relevant the path and a second package:

.. list-table::
   :header-rows: 1

   * - Kind
     - Meaning
   * - ``DB_CHECK_CORRUPT_RECORD``
     - A package record cannot be parsed
   * - ``DB_CHECK_NAME_MISMATCH``
     - A record is stored under a key that differs from the name inside it
   * - ``DB_CHECK_FILES_WITHOUT_PACKAGE``
     - A file list exists for a package that has no record
   * - ``DB_CHECK_FILE_NOT_OWNED``
     - A listed file has no ownership entry
   * - ``DB_CHECK_FILE_OWNED_BY_OTHER``
     - A listed file is recorded as owned by a different package
   * - ``DB_CHECK_OWNER_WITHOUT_PACKAGE``
     - An ownership entry names a package that has no record
   * - ``DB_CHECK_OWNER_NOT_LISTED``
     - An ownership entry names an installed package whose file list lacks
       the file

A database written only through libapg reports nothing. One removed record
can produce several issues, for example a file list and its ownership
entries. The function does not repair anything.

``db_verify()`` checks that every file recorded for each package exists under
a given root and reports the missing ones.

Flags
-----

``db_set_hold()`` and ``db_set_installed_by_hand()`` rewrite the package
record without writing a journal entry. A held package blocks upgrade and
removal in ``trans_prepare()`` with ``TRANS_ERR_HELD``.

Hooks
-----

``db_set_hooks()`` registers ``pre`` and ``post`` callbacks that run around
each ``db_add()`` and ``db_remove()``. They also run for the record rewrite
done by ``db_set_hold()`` and ``db_set_installed_by_hand()``, which are
reported as ``DB_OP_ADD``.
