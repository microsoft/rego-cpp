---
name: update-website
description: 'Update the rego-cpp documentation website to the latest version. Use when: preparing a release, refreshing API docs after code changes, or syncing the website version strings with the VERSION file. Builds Rust, Python, dotnet, and C++ documentation and copies them into the docs/ directory, syncs embedded code samples from the tested examples/ source files, and updates version references in index.html.'
---

# Updating the rego-cpp Documentation Website

Rebuild all API documentation and update version references on the website.

## When to Use

- Preparing a new release (after bumping the VERSION file)
- Refreshing API docs after code changes to any wrapper or the C/C++ API
- Syncing version strings displayed on the website with the VERSION file

## Overview

The website lives in `docs/` at the repo root and is served by GitHub Pages.
It contains:

- `docs/index.html` — Main landing page with examples and version references
- `docs/cpp/` — C/C++ API docs (Doxygen)
- `docs/rust/` — Rust API docs (rustdoc)
- `docs/python/` — Python API docs (Sphinx)
- `docs/dotnet/` — .NET API docs (docfx)

## Procedure

Read the current version from the `VERSION` file before starting. All steps
below use this version string.

### Step 1: Build and Copy Rust Documentation

```bash
cd wrappers/rust/regorust
cargo doc --no-deps
```

Output is generated in `wrappers/rust/regorust/target/doc/`. Copy the
contents into `docs/rust/`, replacing the existing files:

```bash
rm -rf docs/rust/*
cp -r wrappers/rust/regorust/target/doc/* docs/rust/
```

### Step 2: Build and Copy Python Documentation

The Python docs use Sphinx. Build from the `wrappers/python/docs/` directory:

```bash
cd wrappers/python/docs
make html
```

Output is generated in `wrappers/python/docs/build/html/`. Copy the contents
into `docs/python/`, replacing the existing files:

```bash
rm -rf docs/python/*
cp -r wrappers/python/docs/build/html/* docs/python/
```

**Note**: The Python Sphinx config (`wrappers/python/docs/source/conf.py`)
contains a `release` field. Update it to match the VERSION file before
building if it is out of date.

### Step 3: Build and Copy dotnet Documentation

The dotnet docs use [docfx](https://dotnet.github.io/docfx/). Build from
the `wrappers/dotnet/` directory:

```bash
cd wrappers/dotnet
docfx build
```

Output is generated in `wrappers/dotnet/_site/`. Copy the contents into
`docs/dotnet/`, replacing the existing files:

```bash
rm -rf docs/dotnet/*
cp -r wrappers/dotnet/_site/* docs/dotnet/
```

### Step 4: Build and Copy C++ Documentation

The C++ docs use Doxygen and are configured via CMake. The `build-docs`
folder uses Unix Makefiles (not Ninja). Build from the repo root:

```bash
cd build-docs
make regocpp_doc
```

Output is generated in `build-docs/doc/doc/html/`. Copy the contents into
`docs/cpp/`, replacing the existing files:

```bash
rm -rf docs/cpp/*
cp -r build-docs/doc/doc/html/* docs/cpp/
```

### Step 5: Sync Website Code Samples with Example Files

The code samples embedded in `docs/index.html` must match the tested example
source files in the `examples/` directory. Those example files are compiled
and run as part of the normal build/test lifecycle, so keeping the website in
sync with them ensures the displayed code is always tested.

Run the sync script from the repo root:

```bash
python3 .github/skills/update-website/sync_examples.py
```

The script reads each example source file, HTML-escapes it, strips C/C++
copyright headers, and replaces the corresponding `<code>` block in
`docs/index.html`. It identifies the correct block by matching the GitHub
link that follows each code sample.

**C example special handling**: The C example (`examples/c/example.c`)
contains non-rego `#include` directives (e.g., `<stdio.h>`, `<stdlib.h>`)
and helper functions (`print_output`, `print_error`) that add robustness
but clutter the website sample. The script automatically strips these,
keeping only `#include <rego/rego_c.h>` and the body starting from `main()`.

| Website section | Source file | `<code>` class |
|-----------------|------------|----------------|
| `#cpp-example` | `examples/c/example.c` | `language-cpp` |
| `#dotnet-example` | `examples/dotnet/example/Program.cs` | `language-csharp` |
| `#python-example` | `examples/python/example.py` | `language-python` |
| `#rust-example` | `examples/rust/src/main.rs` | `language-rust` |

The script exits with code 0 if all 4 samples were updated, or 1 if any
were skipped or not found. Review its output to confirm all samples matched.

### Step 6: Update Version References in the Website

The file `docs/index.html` contains version strings that must match the
VERSION file. Search for and update all occurrences:

1. **Hero version badge** — look for `Currently <strong>vX.Y.Z</strong>` and
   update to the current version.
2. **Footer version** — look for `Currently vX.Y.Z` in the footer `<ul>` and
   update to the current version.

Both use the format `vX.Y.Z` (with the `v` prefix). Use the version from the
VERSION file, prepended with `v`.

Additionally, check and update any **Rust crate version** reference in the
Rust example section. Look for the `regorust = "X.Y.Z"` line in the
Cargo.toml code example and update it to match the current Rust wrapper
version from `wrappers/rust/regorust/Cargo.toml`.

3. **OPA compatibility version** — look for `compatible with vX.Y.Z of the`
   in the "What is Rego?" section and update it to match
   `REGOCPP_OPA_VERSION` from the root `CMakeLists.txt`.

## Checklist

- [ ] VERSION file read and version noted
- [ ] Rust docs built and copied to `docs/rust/`
- [ ] Python docs built and copied to `docs/python/`
- [ ] Python `conf.py` release field matches VERSION
- [ ] dotnet docs built and copied to `docs/dotnet/`
- [ ] C++ docs built and copied to `docs/cpp/`
- [ ] Website code samples synced from `examples/` source files
- [ ] `docs/index.html` hero version updated
- [ ] `docs/index.html` footer version updated
- [ ] `docs/index.html` Rust crate version updated (if applicable)
- [ ] `docs/index.html` OPA compatibility version matches `REGOCPP_OPA_VERSION`

## Common Mistakes

- **Forgetting to update `conf.py`**: The Python Sphinx config has its own
  `release` field that appears in the generated docs. If this is stale, the
  Python API docs will show the wrong version.
- **Using `ninja` for build-docs**: The `build-docs` folder is configured
  with Unix Makefiles, not Ninja. Use `make`, not `ninja`.
- **Missing `--no-deps` for Rust**: Without `--no-deps`, `cargo doc` builds
  docs for all dependencies, which is slow and unnecessary.
- **Stale build artifacts**: Always `rm -rf` the target `docs/<lang>/*`
  directory before copying to avoid leftover files from previous builds.
- **Copyright year in footer**: The footer contains a copyright year. Update
  it if preparing a release in a new calendar year.
- **Forgetting HTML escaping**: Raw `<`, `>`, and `&` in source code will
  break the HTML. Always escape when embedding into `<code>` blocks.
- **Replacing the wrong code block**: Each language section has two code
  blocks — a short integration snippet (CMake/Cargo/pip) and the main example.
  Only replace the main example block, not the integration snippet.
- **Stale OPA version**: The website states OPA compatibility (e.g.,
  "compatible with vX.Y.Z"). This must match `REGOCPP_OPA_VERSION` in the
  root `CMakeLists.txt`, not the rego-cpp version.
