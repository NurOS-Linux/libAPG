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

Before submitting, run the quality checks:

```bash
python3 scripts/checkpatch.py
```

This runs three checks in sequence:

- **clang-format** — verifies formatting against `.clang-format`
- **SPDX** — every `.c`, `.h`, and `.py` file must have `SPDX-License-Identifier`
  and `SPDX-FileCopyrightText` in the first ten lines
- **Doxygen coverage** — every public function declared in `include/` must have
  a `/** ... */` doc comment

All three must pass. A PR that fails any of these checks will not be merged.

## Documentation

Public API functions in `include/` require a Doxygen comment:

```c
/**
 * Brief one-line description.
 *
 * @param foo  What foo is.
 * @return     What the function returns.
 */
int apg_example(int foo);
```

Internal functions (`src/`) and `static` functions are exempt.

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
