Transactions
============

Atomic multi-package install and remove operations. A transaction resolves
dependencies, detects conflicts, builds an ordered execution plan, and either
commits all steps or rolls back on failure.

Lifecycle
---------

1. :c:func:`trans_new` — allocate a transaction bound to an open database.
2. :c:func:`trans_add_install` / :c:func:`trans_add_remove` — queue operations.
3. :c:func:`trans_prepare` — resolve deps and detect conflicts.
4. :c:func:`trans_get_plan` / :c:func:`trans_get_conflicts` — inspect results.
5. :c:func:`trans_commit` — execute (once only).
6. :c:func:`trans_free` — release resources.

.. doxygenfile:: transaction.h
   :project: libapg
