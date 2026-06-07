Cryptographic primitives
========================

Low-level streaming hash implementations. On supported architectures
(x86-64, aarch64, riscv64) hardware acceleration is used automatically.

All three APIs follow the same init / update / final pattern and provide a
convenience ``compute_*`` function that hashes an entire file in one call.

SHA-256
-------

.. doxygenfile:: sha256.h
   :project: libapg

MD5
---

.. doxygenfile:: md5.h
   :project: libapg

CRC-32
------

.. doxygenfile:: crc32.h
   :project: libapg
