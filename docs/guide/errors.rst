Error handling
==============

Return values
-------------

Functions report failure through their return value: ``false`` for boolean
functions, NULL for functions that return a pointer, and a ``trans_error_t``
for transaction functions. Lookup functions such as ``db_get()`` and
``db_owner()`` also return NULL when nothing is found, which is not an error.

Error message
-------------

When a function fails because of an operating system, database, archive or
script error, it stores a message that names the object and the cause.
``apg_last_error()`` returns it.

- The message is stored per thread.
- The pointer is valid until the next libapg call on the same thread.
- A successful call does not necessarily clear it, so read it only right after
  a call reported failure.
- Not every failure sets a message. Argument checks that return false or NULL
  immediately, and ``package_metadata_from_json()`` and its relatives on
  malformed input, leave it unset.

``unarchive_package()`` and ``unarchive_package_in_root()`` additionally keep
their own message, readable with ``archive_last_error()``. It is cleared at the
start of each of those calls.

Transaction codes
-----------------

``trans_prepare()`` and ``trans_commit()`` return a ``trans_error_t``. The
code says what kind of failure happened and ``apg_last_error()`` adds the
package and cause when there is one. The codes and the lists that explain them
are described in :doc:`transactions`.

Ownership
---------

Arrays returned by ``db_list()``, ``db_search()``, ``db_get_orphans()`` and
``db_get_dependents()`` are heap-allocated. Free every element, then the array.
Packages are released with ``package_free()``, journal entries with
``journal_free_all()``, and verification results with ``db_verify_free()``.
Views returned by ``trans_*_at()`` are owned by the transaction.
