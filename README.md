# libapg

Package management library for NurOS.

> [!NOTE]
> **Repository Mirrors**
>
> - **git.nuros.org** ([core/libapg](https://git.nuros.org/core/libapg)): primary, self-hosted Forgejo instance, accounts restricted to the core team.
> - **GitHub** ([NurOS-Linux/libapg](https://github.com/NurOS-Linux/libapg)): mirror for external contributors. Issues and Pull Requests opened here are welcome and are reviewed and processed by the core team.


## Dependencies

| Dependency | Description |
|------------|-------------|
| [Meson](https://mesonbuild.com/) | Build system |
| [Ninja](https://ninja-build.org/) | Build tool |
| [pkg-config](https://www.freedesktop.org/wiki/Software/pkg-config/) | Helper tool for compiling |
| [libarchive](https://www.libarchive.org/) | Archive and compression library |
| [lmdb](https://www.symas.com/lmdb) | Embedded key-value database |
| [yyjson](https://github.com/ibireme/yyjson) | JSON library |
| [gpgme](https://www.gnupg.org/related_software/gpgme/) **and** [libsodium](https://libsodium.org/) | Package signing |
| [libseccomp](https://github.com/seccomp/libseccomp) *(optional, Linux only)* | Syscall filtering for install script sandbox |

## Signing backends

libapg builds in both signing backends side by side; callers pick which one to use:

- **libsodium** (`sign_verify`, `sign_file`, `keyring_load`, ...; the default): Ed25519 signing. Always ECC, keys are read from `/etc/apg/keys/`.
- **gpgme** (`sign_verify_gpgme`, `sign_file_gpgme`, `keyring_load_gpgme`, ...): PGP signing via GnuPG. Only ECC keys are accepted by default (Ed25519, ECDSA). RSA can be enabled explicitly by passing `allow_rsa = true`.

`install_policy.backend` (`SIGN_BACKEND_SODIUM` by default, or `SIGN_BACKEND_GPGME`) selects which one `trans_commit()` verifies package signatures against.

## Install-script sandbox

`run_script()` isolates pre/post-install scripts using the strongest primitive available on the host:

- **Linux** — `unshare()` with isolated network, mount, UTS, and IPC namespaces, plus optional `libseccomp` syscall filtering.
- **FreeBSD**: `chroot()` into the alternate install root, when one is given. Capsicum capability mode is not used here, since it forbids the kernel from resolving an interpreter path, which any `#!`-script exec requires.
- **Other POSIX platforms** — no sandbox primitive is available; scripts run without isolation.

On Linux and FreeBSD, if the sandbox cannot be established the script is not executed (fail closed).

## Building

### With Nix

```bash
nix build
```

### Without Nix

#### Arch Linux

```bash
sudo pacman -S meson ninja pkgconf libarchive lmdb yyjson gpgme libsodium
```

#### Ubuntu / Debian

```bash
sudo apt install meson ninja-build pkg-config libarchive-dev liblmdb-dev libyyjson-dev libgpgme-dev libsodium-dev
```

#### Fedora / RHEL / CentOS

```bash
sudo dnf install meson ninja-build pkgconf libarchive-devel lmdb-devel yyjson-devel gpgme-devel libsodium-devel
```

#### openSUSE

```bash
sudo zypper install meson ninja pkgconf libarchive-devel lmdb-devel yyjson-devel gpgme-devel libsodium-devel
```

#### Alpine Linux

```bash
sudo apk add meson ninja pkgconf libarchive-dev lmdb-dev yyjson-dev gpgme-dev libsodium-dev
```

#### Gentoo

```bash
sudo emerge dev-build/meson dev-build/ninja dev-util/pkgconf app-arch/libarchive dev-db/lmdb dev-libs/yyjson app-crypt/gpgme dev-libs/libsodium
```

#### Void Linux

```bash
sudo xbps-install meson ninja pkgconf libarchive-devel lmdb-devel gpgme-devel libsodium-devel
```

#### FreeBSD

```bash
sudo pkg install meson pkgconf ninja lmdb libarchive yyjson gpgme libsodium
```

`libseccomp` is not available on FreeBSD; the install-script sandbox falls back to `chroot()` only there (see [Install-script sandbox](#install-script-sandbox)). Default configuration paths follow FreeBSD's `hier(7)` and resolve under `/usr/local/etc/apg/` instead of `/etc/apg/`.

#### Build

```bash
meson setup build --buildtype=release
meson compile -C build
```

#### Install

```bash
sudo meson install -C build
```

## Using libapg

Installing `libapg` also installs a `libapg.pc` pkg-config file and a CMake package config module, so it can be picked up either way:

```bash
pkg-config --cflags --libs libapg
```

```cmake
find_package(libapg REQUIRED)
target_link_libraries(your_target PRIVATE libapg::libapg)
```

The CMake module resolves through pkg-config, so `pkg-config` must be installed and discoverable on the consuming system.

## License

This project is licensed under the **GNU General Public License v3.0** (GPL-3.0).

See the [LICENSE](LICENSE) file for details.
