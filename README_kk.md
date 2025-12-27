# libapg

**Тілдер**: [English](README.md) | [Русский](README_ru.md) | [Қазақша](README_kk.md)

NurOS үшін пакеттерді басқару кітапханасы.

## Құрастыру

### Nix арқылы

```bash
nix build
```

### Nix-сіз

Тәуелділіктерді орнатыңыз:
```bash
# Ubuntu/Debian
sudo apt install meson ninja-build pkg-config libarchive-dev
```

Құрастырыңыз:
```bash
meson setup build --buildtype=release
meson compile -C build
```

**Ескерту**: [libiron](https://github.com/ruzen42/iron-log) қажет

## Лицензия

Copyright © 2025 NurOS
