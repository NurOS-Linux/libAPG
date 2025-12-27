# libapg

**Languages**: [English](README.md) | [Русский](README_ru.md) | [Қазақша](README_kk.md)

Package management library for NurOS.

## Building

### With Nix

```bash
nix build
```

### Without Nix

Install dependencies:
```bash
# Ubuntu/Debian
sudo apt install meson ninja-build pkg-config libarchive-dev
```

Build:
```bash
meson setup build --buildtype=release
meson compile -C build
```

**Note**: Requires [libiron](https://github.com/ruzen42/iron-log)

## License

Copyright © 2025 NurOS
