Install scripts
===============

A package may carry executable scripts in a ``scripts/`` directory. The names
used by the library are ``pre-install``, ``post-install``, ``pre-remove`` and
``post-remove``. Lookup rules are described in :doc:`../api/install`.

When scripts run
----------------

``install_package_in_root()`` runs ``pre-install`` after extraction and before
copying files. It runs ``post-install`` after the ``data/`` and ``home/``
trees are copied. If ``pre-install`` fails, nothing is copied. If
``post-install`` fails, the copied files are removed again.

After a successful install the ``scripts/`` directory is copied to
``<root>/var/lib/apg/pkgscripts/<package>/`` so that ``pre-remove`` and
``post-remove`` can run when the package is removed, after the extracted
package directory is gone. ``scripts_store_path()`` returns that location.
``trans_commit()`` runs ``pre-remove`` before the database record is deleted
and ``post-remove`` after the files are deleted, then removes the stored
scripts. If ``pre-remove`` fails the package stays installed and the commit
returns ``TRANS_ERR_REMOVE_FAILED``.

How a script is executed
------------------------

The script is started in a child process as ``/bin/sh <path>``. The exit
status decides the result: zero is success. A script killed by a signal, or
one that cannot be started, is a failure, and ``apg_last_error()`` describes
it.

When the root is not ``/``, the child calls ``chroot()`` to the root and
changes the directory to ``/``. A script that lives outside the root is first
copied to ``<root>/tmp`` and removed afterwards. If ``chroot()`` fails, for
example for lack of privileges, the script is not run and the call fails.

Isolation
---------

Linux
   The child calls ``unshare()`` with ``CLONE_NEWUSER``, ``CLONE_NEWNET``,
   ``CLONE_NEWNS``, ``CLONE_NEWUTS`` and ``CLONE_NEWIPC`` before running the
   script. If ``unshare()`` fails with ``EPERM`` or ``EINVAL`` (for example
   when unprivileged user namespaces are disabled), the error is ignored and
   the script runs without these namespaces. Any other ``unshare()`` error
   fails the call. System call filtering is not applied: the optional
   libseccomp dependency is detected at build time but not used by the
   current sources.

FreeBSD and other POSIX systems
   FreeBSD runs the script after ``chroot()`` when a root other than ``/`` is
   given. Capsicum is not used, because capability mode forbids resolving the
   interpreter path that a ``#!`` script needs. No namespace or similar
   isolation is applied on these systems. The same code path is used on every
   non-Linux system.

Scripts therefore must not be treated as sandboxed when the host does not
allow ``unshare()``, and a package that is not trusted should be rejected
before installation by requiring signatures (see :doc:`signing`).

Requiring isolation
-------------------

``run_script_strict()`` behaves like ``run_script()`` except that a failed
``unshare()`` is never ignored: the script is not executed, the call returns
false, and ``apg_last_error()`` reports that the script was not run because it
could not be isolated. This covers the ``EPERM`` and ``EINVAL`` cases that
``run_script()`` skips. A missing script is still not an error. On systems
other than Linux no namespace isolation exists, so a script that is present is
never run in strict mode.

``trans_set_require_sandbox(trans, true)`` applies the same rule to every
script that ``trans_commit()`` runs. A step whose script cannot be isolated
fails with ``TRANS_ERR_INSTALL_FAILED`` or ``TRANS_ERR_REMOVE_FAILED`` and the
transaction is rolled back like any other failed step. The default is off, so
existing callers keep the behavior described above.

Strict mode only controls the namespaces. The ``chroot()`` into the target
root is applied in both modes, and the script runs with ``/bin/sh`` from
inside that root, so the root must contain a shell.
