# libapg

Package management library for NurOS.

## Dependencies

| Dependency | Description |
|------------|-------------|
| [Meson](https://mesonbuild.com/) | Build system |
| [Ninja](https://ninja-build.org/) | Build tool |
| [pkg-config](https://www.freedesktop.org/wiki/Software/pkg-config/) | Helper tool for compiling |
| [libarchive](https://www.libarchive.org/) | Archive and compression library |
| [lmdb](https://www.symas.com/lmdb) | Embedded key-value database |
| [yyjson](https://github.com/ibireme/yyjson) | JSON library |
| [gpgme](https://www.gnupg.org/related_software/gpgme/) **or** [libsodium](https://libsodium.org/) | Package signing |

## Signing backends

libapg supports two signing backends. The build system picks the first one available:

- **gpgme** (preferred) — PGP signing via GnuPG. Only ECC keys are accepted by default (Ed25519, ECDSA). RSA can be enabled explicitly by passing `allow_rsa = true` to `sign_verify`.
- **libsodium** (fallback) — Ed25519 signing. Always ECC, keys are read from `/etc/apg/keys/`.

## Building

### With Nix

```bash
nix build
```

### Without Nix

#### Arch Linux

```bash
sudo pacman -S meson ninja pkgconf libarchive lmdb yyjson gpgme
```

#### Ubuntu / Debian

```bash
sudo apt install meson ninja-build pkg-config libarchive-dev liblmdb-dev libyyjson-dev libgpgme-dev
```

#### Fedora / RHEL / CentOS

```bash
sudo dnf install meson ninja-build pkgconf libarchive-devel lmdb-devel yyjson-devel gpgme-devel
```

#### openSUSE

```bash
sudo zypper install meson ninja pkgconf libarchive-devel lmdb-devel yyjson-devel gpgme-devel
```

#### Alpine Linux

```bash
sudo apk add meson ninja pkgconf libarchive-dev lmdb-dev yyjson-dev gpgme-dev
```

#### Gentoo

```bash
sudo emerge dev-build/meson dev-build/ninja dev-util/pkgconf app-arch/libarchive dev-db/lmdb dev-libs/yyjson app-crypt/gpgme
```

#### Void Linux

```bash
sudo xbps-install meson ninja pkgconf libarchive-devel lmdb-devel gpgme-devel
```

#### Build

```bash
meson setup build --buildtype=release
meson compile -C build
```

#### Install

```bash
sudo meson install -C build
```

## License

This project is licensed under the **GNU General Public License v3.0** (GPL-3.0).

See the [LICENSE](LICENSE) file for details.
