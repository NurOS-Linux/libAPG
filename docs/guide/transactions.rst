Transactions
============

A transaction groups installs, upgrades and removals into one plan that is
checked first and then executed in order. The API is declared in
``transaction.h``.

Lifecycle
---------

1. ``trans_new()`` with an open, writable database handle.
2. Optional configuration: ``trans_set_policy()``, ``trans_prefer_provider()``,
   ``trans_set_dry_run()``, ``trans_set_progress_cb()``.
3. Queue work with ``trans_add_install()``, ``trans_add_upgrade()``,
   ``trans_add_remove()`` and, for packages that may be pulled in as
   dependencies, ``trans_add_candidate()``.
4. ``trans_prepare()`` resolves dependencies and builds the plan. On failure it
   returns an error code and the matching list can be inspected.
5. ``trans_commit()`` executes the plan against a root directory.
6. ``trans_free()``.

The transaction borrows the packages passed to it. They must stay valid until
``trans_free()``.

.. code-block:: c

   struct db_handle *db = db_open(db_path);
   struct package *pkg = parse_package(archive_path, root);
   struct apg_trans *trans = trans_new(db);

   trans_add_install(trans, pkg);

   trans_error_t err = trans_prepare(trans);
   if (err == TRANS_OK)
       err = trans_commit(trans, root);
   if (err != TRANS_OK)
       fprintf(stderr, "%s\n", apg_last_error());

   trans_free(trans);
   package_free(pkg);
   db_close(db);

Prepare errors
--------------

``trans_prepare()`` collects every problem it finds before returning. When
more than one kind is present, the code returned follows this priority:

.. list-table::
   :header-rows: 1

   * - Code
     - Inspect with
     - Cause
   * - ``TRANS_ERR_HELD``
     - ``trans_held_pkg_at()``
     - An upgrade or removal targets a package with the held flag
   * - ``TRANS_ERR_FILE_CONFLICT``
     - ``trans_file_conflict_at()``
     - An installed package owns a file that a new package ships
   * - ``TRANS_ERR_HAS_DEPENDENTS``
     - ``trans_blocked_remove_at()``
     - A removal leaves installed packages with a missing dependency
   * - ``TRANS_ERR_CONFLICT``
     - ``trans_conflict_at()``
     - A declared package conflict
   * - ``TRANS_ERR_MISSING_DEP``, ``TRANS_ERR_CYCLE``
     - ``apg_last_error()``
     - Dependency resolution failed

Steps that were not blocked are still planned when the code is
``TRANS_ERR_HELD`` or ``TRANS_ERR_HAS_DEPENDENTS``, but the transaction is not
marked prepared, so ``trans_commit()`` returns ``TRANS_ERR_NOT_PREPARED``.

A file owned by a package that is queued for removal in the same transaction
is not a file conflict.

Plan order
----------

Installs are ordered by dependencies. Upgrades follow. Removals come last,
and a package is removed only after every queued package that depends on it.
Independent removals keep their request order. A dependency cycle among
removals falls back to request order.

``trans_step_required_by()`` tells why a dependency is in the plan. For an
install step that was pulled in as a dependency (``trans_step_explicit()`` is
false) it returns the name of the first step of the plan, install or upgrade,
whose package lists it as a dependency, by name or through ``provides``.
Following the answers from step to step leads to a package the caller asked
for. It returns NULL for steps that were requested directly, for upgrades and
for removals. It is computed from names only, so it does not show which version
constraint caused the dependency, and with ``skip_dependency_check`` every step
is explicit.

Policy
------

``install_policy`` (``config.h``) controls signature checking and two checks
in ``trans_prepare()``:

``require_signature``
   ``trans_commit()`` verifies the detached signature at
   ``<pkg_path>.sig`` for each installed package against the keys in
   ``keyring_dir`` and returns ``TRANS_ERR_UNSIGNED`` on failure. When
   ``keyring_dir`` is NULL, the ``keyring_dir`` build option is used. See
   :doc:`signing`.

``skip_dependency_check``
   Installs are planned as queued, in order, without resolving their
   dependencies.

``skip_dependents_check``
   Removals are planned even when installed packages depend on them.

Policy values are copied by ``trans_set_policy()``. Passing NULL clears the
policy.

Disk space
----------

``trans_estimate_space()`` returns an estimate of the space a prepared
transaction needs and the space available, so a caller can refuse to commit
when ``needed`` exceeds ``available``. It reads the archives of the queued
installs and upgrades but writes nothing.

The estimate adds, for each install and upgrade, the files of the ``data/``
and ``scripts/`` trees; the temporary extraction of the largest archive; and,
for upgrades, a backup of the installed version's files and persisted scripts.
Every file is rounded up to 4096 bytes. It is deliberately conservative:
files replaced by an upgrade are counted twice, removals free nothing in the
estimate, and the ``home/`` tree, which is installed outside the root, is not
counted. When the temporary area and the root are on different file systems,
the pair with the smaller margin is reported. The result is an estimate, not
a guarantee: other processes may use the space before the commit.

Commit
------

``trans_commit()`` may be called once on a prepared transaction. A second call
returns ``TRANS_ERR_ALREADY_COMMITTED``.

Install steps extract the archive, run ``pre-install``, copy the ``data/``
tree into the root, run ``post-install`` and record the package in the
database. Upgrade steps first copy the files and persisted scripts of the
installed version into a temporary backup under the ``tmp_dir`` build option,
so free space for those files is needed while the transaction runs. Files that
the installed version shipped and the new one does not are removed, except
configuration files listed in the old version's ``conf`` entries. Whether the
package was installed explicitly or as a dependency is kept.

Removal steps run ``pre-remove``, delete the record, delete the files the
package still owns, and run ``post-remove``. A file that another package took
over earlier in the same transaction is left in place.

If a step fails, the steps already done are rolled back: new files are removed,
upgraded packages are restored from their backup, and database records are
restored. The code returned is ``TRANS_ERR_INSTALL_FAILED``,
``TRANS_ERR_REMOVE_FAILED`` or ``TRANS_ERR_DB``, and ``apg_last_error()`` names
the package and the cause.

Dry run
-------

With ``trans_set_dry_run(trans, true)``, ``trans_commit()`` verifies
signatures and returns the code a real commit would, but writes and removes
nothing, runs no scripts, and leaves the database and journal untouched. A
dry-run commit does not use up the single commit, so the same transaction can
be committed for real afterwards. The progress callback is called in dry-run
mode as well.

Providers and candidates
------------------------

When a dependency or virtual name has several providers, ``trans_prepare()``
picks an installed one if there is one, otherwise the first provider added to
the transaction. ``trans_prefer_provider()`` overrides this for a name.

A candidate added with ``trans_add_candidate()`` is planned only if something
else in the transaction depends on it, and then as a non-explicit install.
