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
4. :c:func:`trans_plan_at` / :c:func:`trans_conflict_at` — inspect results.
5. :c:func:`trans_commit` — execute (once only).
6. :c:func:`trans_free` — release resources.

.. c:type:: void (*trans_progress_fn)(const struct trans_step *step, size_t index, size_t total, void *userdata)

   Callback invoked by :c:func:`trans_commit` before each plan step runs.
   ``step`` is the step about to be executed, ``index`` its zero-based position
   in the plan, ``total`` the number of steps in the plan, and ``userdata`` the
   pointer passed to :c:func:`trans_set_progress_cb`.

.. doxygenfile:: transaction.h
   :project: libapg
   :sections: briefdescription detaileddescription innerclass define prototype enum func var
