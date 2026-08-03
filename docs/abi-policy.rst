ABI policy
==========

libapg is pre-2.0 and does not yet make a formal ABI stability promise.
This page records the policy decisions made so far, so that stabilization
work happens deliberately rather than by accident.

Frozen: the package format
--------------------------

:c:struct:`package`, :c:struct:`package_metadata`, :c:struct:`str_list`,
:c:struct:`dep_constraint`, and :c:struct:`dep_constraint_list`
(``include/apg/package.h``, ``include/apg/version.h``) mirror the on-disk
``.apg`` package format (``metadata.json`` and the archive layout). That
format is implemented and fixed. These types are not in scope for
ABI-stabilization work, ever — changing their layout is only ever done as a
consequence of a deliberate package-format change, never as a side effect
of an API/ABI cleanup.

Opaque: read-only result types
-------------------------------

Types the caller only ever receives from a query function and never
constructs by hand are made opaque, with accessor functions for every
field:

- :c:struct:`trans_step`, :c:struct:`trans_conflict`,
  :c:struct:`trans_file_conflict`, :c:struct:`trans_held_pkg`,
  :c:struct:`trans_blocked_remove` (``include/apg/transaction.h``)
- :c:struct:`db_verify_issue` (``include/apg/db.h``)
- :c:struct:`journal_entry` (``include/apg/journal.h``)

Callers get them through a ``_count()``/``_at()`` pair
(:c:func:`trans_plan_count`/:c:func:`trans_plan_at` and equivalents) rather
than a raw array pointer, since an opaque type can't be indexed directly.
Adding a field to any of these later is not an ABI break.

Already opaque: handles
------------------------

:c:struct:`db_handle`, :c:struct:`keyring`, :c:struct:`dep_graph`, and
:c:struct:`apg_trans` were designed opaque from the start (forward-declared
in the public header, defined only in the matching ``*_priv.h``). No
action needed here.

Left flat, by design
---------------------

:c:struct:`db_hooks` (``include/apg/db.h``) is a small, purpose-built
callback-registration struct copied by :c:func:`db_set_hooks`; it is not
expected to grow. :c:struct:`db_stats` is a two-field counter struct
populated through an out-parameter; hiding it behind accessors would add
ceremony without reducing any real risk. ``install_policy``
(``include/apg/config.h``) is a small, caller-constructed struct passed to
:c:func:`trans_set_policy`; the same reasoning applies, but unlike
``db_hooks``/``db_stats`` its layout has already changed once (the gpgme
backend-selection field was removed) — a real, deliberate ABI break,
correctly caught by the ``abi-check`` CI job described below.

Policy for new public types
-----------------------------

A new public struct defaults to opaque with accessors unless it is a
small, caller-constructed value type with a field set that is not
expected to grow (matching the reasoning above for ``dep_constraint`` and
``db_hooks``). When in doubt, opaque is the safer default.

Symbol visibility
------------------

``libapg.so`` is built with ``gnu_symbol_visibility: 'hidden'``
(``meson.build``), so every symbol is hidden from the dynamic symbol
table by default. Public functions are exported explicitly with the
``APG_API`` macro (``include/apg/export.h``), applied to every
declaration in ``include/apg/*.h`` and ``include/util.h``. A new
internal helper shared across translation units therefore stays hidden
automatically unless it is deliberately declared in a public header
with ``APG_API`` — there is no separate step to remember, unlike
marking individual internal symbols hidden by hand.

New public functions must add ``APG_API`` to their declaration in the
relevant public header, or they will silently fail to link for external
consumers despite being documented.

Automated ABI regression check
--------------------------------

The ``abi-check`` CI job (``.github/workflows/ci.yml``,
``.forgejo/workflows/ci.yml``) builds the current commit and the previous
release tag (``git describe --tags --abbrev=0``) side by side, dumps each
with ``abidw``, and compares them with::

    abidiff --suppressions abi-suppressions.txt --exported-interfaces-only \
        baseline.xml current.xml

``abidiff``'s exit status is a bitmask; the job fails only when bit 8
(incompatible change) is set, so purely additive changes (new symbols) pass
without any action needed. ``abi-suppressions.txt`` excludes the "Already
opaque: handles" and "Opaque: read-only result types" structs listed above
from the comparison, since ``abidw`` reads full DWARF debug info and would
otherwise flag private-field changes inside these deliberately-opaque
structs as false-positive breaks — something direct callers can never
observe, since they only ever see a forward declaration.

When the job fails on a genuine, intentional ABI break, document it under
a ``Breaking:`` bullet in ``CHANGELOG.md`` and bump the version in
``meson.build`` before release (``soversion`` tracks the full project
version, so the SONAME changes with it — see the soversion fix in
``CHANGELOG.md``).
