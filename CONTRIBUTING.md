# Contributing to libapg

## Getting started

1. Fork the [repository](https://github.com/NurOS-Linux/libAPG)
2. Clone your fork:
   ```bash
   git clone https://github.com/NurOS-Linux/libAPG.git
   cd libAPG
   ```
3. Install dependencies (see [README.md](README.md))

## Building for development

```bash
meson setup build --buildtype=debug
meson compile -C build
```

## Submitting changes

1. Create a branch:
   ```bash
   git checkout -b feat/your-feature
   ```
2. Commit using [Conventional Commits](https://www.conventionalcommits.org/):
   - `feat:` — new feature
   - `fix:` — bug fix
   - `docs:` — documentation only
   - `refactor:` — restructuring without behaviour change
   - `test:` — adding or updating tests
   - `chore:` — maintenance
3. Push and open a Pull Request at https://github.com/NurOS-Linux/libAPG

## Code style

- Indentation: 4 spaces
- All identifiers in English
- No decorative comment separators (`// ---`, `/* === */`, etc.)

## Signing backend

If you contribute to signing code, keep both backends (`src/sign/pgp/` and `src/sign/sodium/`) in sync. The public interface is defined in `include/apg/sign.h` and must not diverge between backends.

## Reporting issues

Open an issue at https://github.com/NurOS-Linux/libAPG/issues and include:

- Operating system and version
- Steps to reproduce the problem
- Expected vs actual behavior
- Relevant error output

## License

By contributing, you agree that your contributions will be licensed under [GPL-3.0](LICENSE).
