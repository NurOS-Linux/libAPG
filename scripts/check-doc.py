#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
# SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>

import re
import sys
from pathlib import Path

KEYWORDS = frozenset([
    'if', 'for', 'while', 'switch', 'return', 'sizeof', 'typeof', 'defined',
])

DECL_RE = re.compile(
    r'^\s*(?!static\s)(?!typedef\s)(?!#)'
    r'(?:[\w\s*]+?)\s+'
    r'(?P<name>[a-z_][a-z0-9_]*)\s*\([^)]*\)\s*;',
    re.MULTILINE,
)


def scan(text: str):
    """Yield (event, line_no) where event is 'doc_end' or ('decl', name)."""
    lines = text.splitlines(keepends=True)
    in_block = False
    is_doc = False
    buf = []
    buf_start = 0

    for lineno, line in enumerate(lines, 1):
        stripped = line.strip()

        if in_block:
            if '*/' in line:
                in_block = False
                if is_doc:
                    yield ('doc_end', lineno)
            continue

        if stripped.startswith('/**'):
            if '*/' in stripped[3:]:
                yield ('doc_end', lineno)
            else:
                in_block = True
                is_doc = True
            continue

        if stripped.startswith('/*'):
            if '*/' not in stripped[2:]:
                in_block = True
                is_doc = False
            continue

        if stripped.startswith('//') or stripped.startswith('#'):
            continue

        buf.append((lineno, line))

        joined = ''.join(l for _, l in buf)
        if ';' in joined or '{' in joined:
            for m in DECL_RE.finditer(joined):
                name = m.group('name')
                if name not in KEYWORDS:
                    yield ('decl', buf[0][0], name)
            buf = []


def check_file(path: Path) -> list[str]:
    text = path.read_text(errors='replace')
    errors = []
    last_doc_end = -1

    for event in scan(text):
        if event[0] == 'doc_end':
            last_doc_end = event[1]
        elif event[0] == 'decl':
            _, lineno, name = event
            if lineno - last_doc_end > 2:
                errors.append(f'{path}:{lineno}: {name}() has no Doxygen comment')

    return errors


if __name__ == '__main__':
    import argparse
    ap = argparse.ArgumentParser()
    ap.add_argument('paths', nargs='*', default=['include'])
    roots = ap.parse_args().paths

    issues = []
    for r in roots:
        p = Path(r)
        files = [p] if p.is_file() else p.rglob('*.h')
        for f in files:
            if f.is_file():
                issues.extend(check_file(f))

    for i in issues:
        print(i)
    sys.exit(1 if issues else 0)
