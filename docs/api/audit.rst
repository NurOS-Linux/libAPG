Audit log
=========

Read access to the operation history through an open database handle.
``audit_read_all()`` wraps ``journal_read_all()`` so callers do not need the
underlying LMDB environment. Entries are returned in chronological order and
are released with ``journal_free_all()``.

``audit_read_since()`` returns the entries whose timestamp is at or after a
given time, and ``audit_read_for_package()`` the entries of one package, matched
by exact name. Both keep chronological order, return NULL when nothing matches
and release their result with ``journal_free_all()``. They read the whole log
and filter it, so the cost grows with the size of the journal.

See :doc:`journal` for the entry accessors and the record format.

.. doxygenfile:: audit.h
   :project: libapg
