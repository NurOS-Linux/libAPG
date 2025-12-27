# libapg

**Языки**: [English](README.md) | [Русский](README_ru.md) | [Қазақша](README_kk.md)

Библиотека управления пакетами для NurOS.

## Зависимости

| Зависимость | Описание |
|-------------|----------|
| [Meson](https://mesonbuild.com/) | Система сборки |
| [Ninja](https://ninja-build.org/) | Инструмент сборки |
| [pkg-config](https://www.freedesktop.org/wiki/Software/pkg-config/) | Вспомогательный инструмент для компиляции |
| [libarchive](https://www.libarchive.org/) | Библиотека для работы с архивами |
| [libiron](https://github.com/ruzen42/iron-log) | Библиотека логирования |

## Сборка

### С Nix

```bash
nix build
```

### Без Nix

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

**Примечание**: Также необходимо вручную собрать и установить [libiron](https://github.com/ruzen42/iron-log).

#### Сборка

```bash
meson setup build --buildtype=release
meson compile -C build
```

#### Установка

```bash
sudo meson install -C build
```

## Лицензия

Проект распространяется под лицензией **GNU General Public License v3.0** (GPL-3.0).

Подробности смотрите в файле [LICENSE](LICENSE).

Copyright (C) 2025 NurOS
