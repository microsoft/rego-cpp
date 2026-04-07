#!/usr/bin/env python3
# Copyright (c) Microsoft Corporation.
# Licensed under the MIT License.

"""Sync website code samples in docs/index.html with the tested example files.

Usage (from repo root):
    python3 .github/skills/update-website/sync_examples.py

Each example source file is read, HTML-escaped, and injected into the
corresponding <code> block in docs/index.html.  The script identifies the
correct block by matching the GitHub link that follows each code sample.
"""

import html
import re
import sys
from pathlib import Path

# Repo root is three levels up from this script
REPO_ROOT = Path(__file__).resolve().parents[3]
INDEX_HTML = REPO_ROOT / "docs" / "index.html"

# Copyright header stripped from C/C++ files before embedding
COPYRIGHT_RE = re.compile(
    r"^// Copyright \(c\) Microsoft Corporation\.\n"
    r"// Licensed under the MIT License\.\n\n?",
)

# Non-rego #include lines stripped from C examples (keep only rego_c.h)
C_NON_REGO_INCLUDE_RE = re.compile(r"^#include <(?!rego/).*>\n", re.MULTILINE)

# Helper function definitions stripped from C examples.
# Matches function definitions (return-type name(args) { ... }) that appear
# before main().  We detect them as blocks starting with a type + identifier
# pattern and ending at a closing brace at column 0.
C_HELPER_FN_RE = re.compile(
    r"^(?!int main\b)\w[\w *]*\b\w+\(.*?\)\n\{.*?^}\n\n?",
    re.MULTILINE | re.DOTALL,
)

# (source file relative to repo root, language class, GitHub link suffix)
EXAMPLES = [
    (
        "examples/c/example.c",
        "language-cpp",
        "examples/c/example.c",
    ),
    (
        "examples/dotnet/example/Program.cs",
        "language-csharp",
        "examples/dotnet/Program.cs",
    ),
    (
        "examples/python/example.py",
        "language-python",
        "examples/python/example.py",
    ),
    (
        "examples/rust/src/main.rs",
        "language-rust",
        "examples/rust/src/main.rs",
    ),
]


def build_pattern(lang_class: str, link_suffix: str) -> re.Pattern:
    """Build a regex that matches the <code> block followed by its GitHub link."""
    escaped_suffix = re.escape(link_suffix)
    return re.compile(
        rf'(<code class="{re.escape(lang_class)}">)'
        rf"(.*?)"
        rf"(</code></pre>\s*<a\s+href=\"https://github\.com/microsoft/rego-cpp/blob/main/{escaped_suffix}\">)",
        re.DOTALL,
    )


def main() -> int:
    if not INDEX_HTML.exists():
        print(f"ERROR: {INDEX_HTML} not found", file=sys.stderr)
        return 1

    content = INDEX_HTML.read_text(encoding="utf-8")
    ok = 0

    for source_rel, lang_class, link_suffix in EXAMPLES:
        source_path = REPO_ROOT / source_rel
        if not source_path.exists():
            print(f"SKIP: {source_rel} (file not found)")
            continue

        source = source_path.read_text(encoding="utf-8")

        # Strip copyright headers from C/C++ files
        source = COPYRIGHT_RE.sub("", source)

        # Special handling for the C example: strip non-rego includes and
        # helper functions to keep the website sample focused on the API.
        if source_rel == "examples/c/example.c":
            source = C_NON_REGO_INCLUDE_RE.sub("", source)
            source = C_HELPER_FN_RE.sub("", source)
            # Collapse any resulting leading blank lines
            source = source.lstrip("\n")

        # HTML-escape and trim trailing whitespace
        escaped = html.escape(source).rstrip()

        pattern = build_pattern(lang_class, link_suffix)
        match = pattern.search(content)
        if match:
            content = (
                content[: match.start()]
                + match.group(1)
                + escaped
                + match.group(3)
                + content[match.end() :]
            )
            print(f"  OK: {source_rel}")
            ok += 1
        else:
            print(f"MISS: {source_rel} (pattern not found in index.html)")

    INDEX_HTML.write_text(content, encoding="utf-8")
    print(f"\nUpdated {ok}/{len(EXAMPLES)} code samples in {INDEX_HTML}")
    return 0 if ok == len(EXAMPLES) else 1


if __name__ == "__main__":
    sys.exit(main())
