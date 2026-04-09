---
name: bump-version
description: 'Bump the rego-cpp version number for a new release. Use when: preparing a release, updating version strings after a tag, or when instructed to bump the version. Updates all version files across the main library and wrapper packages to keep them in sync.'
---

# Bumping the Version

Update all version strings across the rego-cpp project for a new release.

## When to Use

- Preparing a new release (major, minor, or patch)
- After discovering wrapper versions are out of sync with the main VERSION file
- When instructed to bump versions

## Files to Update

Every release requires updating version strings in **all** of the following
locations. Missing any one of them creates a version mismatch between the
library and its wrapper packages.

| File | Field / Pattern | Example |
|------|----------------|---------|
| `VERSION` | Entire file contents | `1.3.0` |
| `ports/rego-cpp/vcpkg.json` | `"version": "X.Y.Z"` | `"version": "1.3.0"` |
| `wrappers/python/setup.py` | `VERSION = "X.Y.Z"` | `VERSION = "1.3.0"` |
| `wrappers/rust/regorust/Cargo.toml` | `version = "X.Y.Z"` | `version = "1.3.0"` |
| `wrappers/dotnet/Rego/Rego.csproj` | `<Version>X.Y.Z</Version>` | `<Version>1.3.0</Version>` |
| `examples/rust/Cargo.toml` | `regorust = { version = "X.Y.Z" }` | `regorust = { version = "1.3.0" }` |
| `examples/dotnet/example/example.csproj` | `Version="X.Y.Z"` | `Version="1.3.0"` |
| `examples/dotnet/MyPolicy/MyPolicy.csproj` | `Version="X.Y.Z"` | `Version="1.3.0"` |

## Procedure

1. Read the current version from the `VERSION` file at the repo root.
2. Determine the new version (from user instruction or by incrementing).
3. Update all four files listed above.
4. Verify no other files reference the old version:
   ```bash
   grep -rn '"OLD_VERSION"' wrappers/ VERSION
   ```
5. Update the CHANGELOG with the new version header if not already present.

## OPA Version Updates

When the OPA compatibility version changes (i.e., `REGOCPP_OPA_VERSION` in
the root `CMakeLists.txt` is updated), the following file must also be
updated to stay in sync:

| File | Field / Pattern | Example |
|------|----------------|---------|
| `.github/copilot-instructions.md` | `targets Rego vX.Y.Z` | `targets Rego v1.15.1` |

This is separate from the rego-cpp version bump — `REGOCPP_OPA_VERSION`
tracks which OPA release rego-cpp is compatible with, and can change
independently.

## Common Mistakes

- **Forgetting wrapper versions**: The wrapper packages (Python, Rust, .NET)
  each have their own version string that must match the main `VERSION` file.
  These are easy to miss because they live in different directories and formats.
- **Cargo.lock stale**: After updating `Cargo.toml`, run `cargo update` in the
  Rust wrapper directory if a lockfile exists, or CI may fail.
