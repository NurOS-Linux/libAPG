Utilities
=========

Small path and file helpers shared by the library and exported for callers.

``concat_dirs()`` always inserts a separator between its arguments and
collapses repeated ``/`` in the result. ``collect_files()`` returns paths
relative to ``base`` that start with ``/``, for example ``/usr/bin/tool``,
and lists symbolic links without following them.

.. doxygenfile:: util.h
   :project: libapg
