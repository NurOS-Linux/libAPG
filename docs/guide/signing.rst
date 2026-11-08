Signing and keys
================

Signatures use Ed25519 through libsodium. A package signature is a detached
file of 64 raw bytes. Keys are raw bytes, not encoded: a public key is 32
bytes and a secret key is 64 bytes, as produced by ``crypto_sign_keypair()``.
libapg does not include a key generation tool.

Two interfaces
--------------

``sign_file()`` and ``sign_verify()`` (``sign.h``) work with one fixed key
pair. They read ``secret.key`` and ``public.key`` from the ``keys_dir`` build
option (``/etc/apg/keys`` by default). ``sign_verify()`` accepts a signature
made with the matching secret key only. The ``allow_rsa`` argument is ignored.

``sign_file_with_key()`` and ``sign_verify_with_key()`` do the same with key
files given by path, so they do not depend on the ``keys_dir`` build option.
The signature format is identical: a file made by ``sign_file_with_key()``
verifies with ``sign_verify()`` and ``keyring_verify()`` when the matching
public key is installed. NULL arguments make them return false.

``keyring_load()`` and ``keyring_verify()`` (``keyring.h``) work with a
directory of trusted public keys. Every file whose name ends in ``.key`` and
that holds at least 32 bytes contributes one key (the first 32 bytes). Other
files are ignored. ``keyring_verify()`` accepts a signature if any key in the
keyring verifies it. This is the interface used by transactions.

What is signed
--------------

The signature covers the whole package file, hashed in a streaming fashion
with ``crypto_sign_init()``, ``crypto_sign_update()`` and
``crypto_sign_final_create()``. This is the Ed25519ph variant. A signature
made with ``crypto_sign_detached()`` over the same bytes does not verify.

For a package at ``pkg_path`` a transaction looks for the signature at
``pkg_path.sig``.

Requiring signatures
--------------------

Set ``require_signature`` in the install policy (see :doc:`transactions`).
``trans_commit()`` then loads the keyring from the policy's ``keyring_dir``,
or from the ``keyring_dir`` build option when it is NULL, and returns
``TRANS_ERR_UNSIGNED`` if the keyring cannot be loaded, the signature file is
missing, or no key verifies it. In dry-run mode the check still runs.

Adding keys
-----------

``keyring_add_key()`` adds a public key to a keyring directory only if an
existing trusted key endorses it. The endorsement is a detached signature file
of 64 bytes made with ``crypto_sign_detached()`` over the 32 bytes of the new
key. The key is written to the keyring directory under the base name of the
source path. A ``.key`` extension is appended when the name does not already
end in it, because ``keyring_load()`` ignores other names. An existing file of
the same name is overwritten.

The function takes the trusted keyring as an argument and returns false when
the endorsement does not verify, when any file cannot be read, or when the
file cannot be written.
