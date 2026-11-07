Package database
================

LMDB-backed store for installed packages. Supports read-write and read-only
handles, pre/post write hooks, and file-ownership queries.

.. c:type:: void (*db_hook_fn)(db_op_t op, const char *pkg_name, void *userdata)

   Callback signature for pre/post write hooks. ``op`` is the operation being
   performed, ``pkg_name`` is the name of the package being added or removed,
   and ``userdata`` is the pointer stored in :c:struct:`db_hooks`.

.. doxygenfile:: db.h
   :project: libapg
   :sections: briefdescription detaileddescription innerclass define prototype enum func var
