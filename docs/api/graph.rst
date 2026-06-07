Dependency graph
================

Low-level dependency resolution, cycle detection, and conflict analysis.
The transaction layer uses this internally; direct use is for advanced
tooling that needs to inspect the resolution graph.

.. doxygenfile:: graph.h
   :project: libapg
