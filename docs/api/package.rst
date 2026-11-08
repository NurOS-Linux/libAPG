Package types
=============

Core types representing packages and their metadata.

Inspecting archives
-------------------

``package_read_metadata()`` and ``package_list_files()`` read a package
archive without extracting it and without writing anything to disk. The
first returns the contents of ``metadata.json`` (at most 1 MiB). The second
returns the paths of everything under the archive's ``data/`` tree that is not
a directory, in the same form that ``install_package_in_root()`` records in
``package_files``: paths start with ``/`` and are relative to the install root.
A path with a ``..`` component makes ``package_list_files()`` fail. Both read
gzip, xz and zstd compressed tar archives, like the extraction functions, and
set ``apg_last_error()`` on failure.

.. doxygenfile:: package.h
   :project: libapg
