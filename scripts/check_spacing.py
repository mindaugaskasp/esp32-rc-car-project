#!/usr/bin/env python3
"""Fails if any C/C++ source line uses column-alignment padding — two or more
consecutive spaces inside code, outside string literals and comments.

Enforces the single-space rule from CLAUDE.md ("No column-alignment padding"):
one space around '=' and between a type and its name; never pad to line up
columns. Run by `make check` / before every build.
"""
import re
import sys
import pathlib

SOURCE_SUFFIXES = (".h", ".cpp")


def code_only(line: str) -> str:
    """Strip string/char literals, // comments, and single-line /* */ blocks so
    only the executable code remains for the padding check."""
    without_strings = re.sub(r'"(\\.|[^"\\])*"', '""', line)
    without_chars = re.sub(r"'(\\.|[^'\\])*'", "''", without_strings)
    without_block = re.sub(r"/\*.*?\*/", "", without_chars)
    without_line_comment = re.sub(r"//.*$", "", without_block)
    return without_line_comment


def main() -> int:
    offenders = []
    for path in sorted(pathlib.Path("src").rglob("*")):
        if path.suffix not in SOURCE_SUFFIXES:
            continue
        for lineno, raw in enumerate(path.read_text().splitlines(), 1):
            code = code_only(raw)
            body = code.lstrip()          # ignore leading indentation
            if body.startswith("*"):      # block-comment continuation line
                continue
            if re.search(r"\S {2,}\S", body):
                offenders.append(f"  {path}:{lineno}: {raw.strip()}")

    if offenders:
        print("Column-alignment padding found — use single spaces (see CLAUDE.md):")
        print("\n".join(offenders))
        return 1
    print("Spacing OK: no column-alignment padding.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
