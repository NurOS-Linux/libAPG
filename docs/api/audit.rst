Audit log
=========

Read access to the operation history through an open database handle.
``audit_read_all()`` wraps ``journal_read_all()`` so callers do not need the
underlying LMDB environment. Entries are returned in chronological order and
are released with ``journal_free_all()``.

See :doc:`journal` for the entry accessors and the record format.

.. doxygenfile:: audit.h
   :project: libapg
