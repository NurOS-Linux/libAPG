# libapg

**Языки**: [English](README.md) | [Русский](README_ru.md) | [Қазақша](README_kk.md)

Библиотека управления пакетами для NurOS.

## Сборка

### С Nix

```bash
nix build
```

### Без Nix

Установите зависимости:
```bash
# Ubuntu/Debian
sudo apt install meson ninja-build pkg-config libarchive-dev
```

Соберите:
```bash
meson setup build --buildtype=release
meson compile -C build
```

**Примечание**: Требуется [libiron](https://github.com/ruzen42/iron-log)

## Лицензия

Copyright © 2025 NurOS
