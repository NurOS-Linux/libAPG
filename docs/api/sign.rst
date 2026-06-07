Signing
=======

Package signature creation and verification. Two backends are supported and
selected at build time: **gpgme** (preferred) and **libsodium** (fallback).

Signature functions
-------------------

.. doxygenfile:: sign.h
   :project: libapg

Keyring management
------------------

.. doxygenfile:: keyring.h
   :project: libapg
