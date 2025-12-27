# libapg

**Тілдер**: [English](README.md) | [Русский](README_ru.md) | [Қазақша](README_kk.md)

NurOS үшін пакеттерді басқару кітапханасы.

## Тәуелділіктер

| Тәуелділік | Сипаттама |
|------------|-----------|
| [Meson](https://mesonbuild.com/) | Құрастыру жүйесі |
| [Ninja](https://ninja-build.org/) | Құрастыру құралы |
| [pkg-config](https://www.freedesktop.org/wiki/Software/pkg-config/) | Компиляция үшін көмекші құрал |
| [libarchive](https://www.libarchive.org/) | Архивтермен жұмыс істеу кітапханасы |
| [libiron](https://github.com/ruzen42/iron-log) | Логтау кітапханасы |

## Құрастыру

### Nix арқылы

```bash
nix build
```

### Nix-сіз

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

**Ескерту**: Сондай-ақ [libiron](https://github.com/ruzen42/iron-log) кітапханасын қолмен құрастырып орнату қажет.

#### Құрастыру

```bash
meson setup build --buildtype=release
meson compile -C build
```

#### Орнату

```bash
sudo meson install -C build
```

## Лицензия

Бұл жоба **GNU General Public License v3.0** (GPL-3.0) лицензиясымен таратылады.

Толық ақпарат үшін [LICENSE](LICENSE) файлын қараңыз.

Copyright (C) 2025 NurOS
