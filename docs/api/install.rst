Installation
============

Low-level file installation and rollback. These functions operate on an
already-extracted package directory.

File installation
-----------------

.. doxygenfile:: install.h
   :project: libapg

Directory copy
--------------

``copy_dir()`` merges into an existing destination. Files that exist only in
the destination are kept. When an entry has a different type in the source and
the destination (file, directory, symbolic link), the destination entry is
replaced. Symbolic links are recreated, not followed. Permission bits of files
and newly created directories are copied from the source. Ownership is not
copied. Entries that are neither regular files, directories nor symbolic links
are skipped.

The single-file helper ``copy_file()`` is declared in :doc:`util`.

.. doxygenfile:: copy.h
   :project: libapg

Lifecycle scripts
-----------------

``run_script()`` looks in ``<pkg_dir>/scripts/`` for an entry whose name
matches ``name`` after lower-casing and dropping ``-`` and ``_``, so
``pre-install``, ``pre_install`` and ``PreInstall`` all match. Entries whose
name starts with ``.`` are ignored. The call returns true when the
``scripts/`` directory, the matching entry or its execute permission is
missing. The script is started as ``/bin/sh <path>``, so a ``#!`` line is not
used. The isolation applied to scripts is described in
:doc:`../guide/install-scripts`.

.. doxygenfile:: scripts.h
   :project: libapg
