# libapg

**Languages**: [English](README.md) | [Русский](README_ru.md) | [Қазақша](README_kk.md)

Package management library for NurOS.

## Dependencies

| Dependency | Description |
|------------|-------------|
| [Meson](https://mesonbuild.com/) | Build system |
| [Ninja](https://ninja-build.org/) | Build tool |
| [pkg-config](https://www.freedesktop.org/wiki/Software/pkg-config/) | Helper tool for compiling |
| [libarchive](https://www.libarchive.org/) | Archive and compression library |
| [libiron](https://github.com/ruzen42/iron-log) | Logging library |

## Building

### With Nix

```bash
nix build
```

### Without Nix

#### Arch Linux / NurOS

```bash
sudo pacman -S meson ninja pkgconf libarchive
```

#### Ubuntu / Debian

```bash
sudo apt install meson ninja-build pkg-config libarchive-dev
```

#### Fedora / RHEL / CentOS

```bash
sudo dnf install meson ninja-build pkgconf libarchive-devel
```

#### openSUSE

```bash
sudo zypper install meson ninja pkgconf libarchive-devel
```

#### Alpine Linux

```bash
sudo apk add meson ninja pkgconf libarchive-dev
```

#### Gentoo

```bash
sudo emerge dev-build/meson dev-build/ninja dev-util/pkgconf app-arch/libarchive
```

#### Void Linux

```bash
sudo xbps-install meson ninja pkgconf libarchive-devel
```

**Note**: You also need to build and install [libiron](https://github.com/ruzen42/iron-log) manually.

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

Copyright (C) 2025 NurOS
