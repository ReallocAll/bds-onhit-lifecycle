# BDS OnHit lifecycle diagnostics

**Experimental; not verified on BDS. Not a leak fix.** Private engineering repository for BDS 1.26.60.6, Endstone C++ plugin API v0.11. The goal is to distinguish actual `ProjectileComponent` / `OnHitSubcomponent` retention from allocator sampling, caches and ownership aliases.

## Scope and safety

- Never install or run this plugin on production. Production is **strictly read-only**.
- The only target binary is SHA-256 `ea16b541a448e07c337848d2c5f37321b706554b5216dd942f3f11e7a8ed2bad`, GNU Build ID `52576b75b6af5a09a6bde582be804e29657dbbf3`.
- The plugin hashes the **running executable** via `/proc/self/exe`, checks the Build ID from the loaded ELF, and uses its load bias to resolve candidate EAs. Identity mismatch disables diagnostics.
- **Default build never installs hooks.** Experimental detours are compile-time disabled; see ABI blockers before considering an isolated test.
- No deletion, allocator interception or memory patch is implemented. Decreased sample count is not proof of correct ownership.
- No paid CI or cloud infrastructure may be used; confirm free budget before any Actions run.

## Build

On Linux x86-64, use clang 18+, libc++, CMake, Ninja and Conan 2 compatible with Endstone. The project pins Endstone tag `v0.11.15`; CMake FetchContent needs network. Install OpenSSL development libraries. Experimental hooks additionally need Conan `funchook/1.1.3` and a configured CMake package path.

```sh
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build build
```

The resulting plugin should be named `endstone_onhit_lifecycle.so`, subject to Endstone's build rules. Put it under an **isolated test server** `plugins/` directory. This default variant only reports whether the binary identity matches, **without recording live lifecycle events**.

Offline component tests, with **no Endstone dependency**:

```sh
cmake -S . -B build-tests -DONHIT_BUILD_PLUGIN=OFF -DONHIT_BUILD_TESTS=ON
cmake --build build-tests -j
ctest --test-dir build-tests --output-on-failure
```

## Experimental instrumentation (blocked until ABI/unwind verification)

Only after verifying all static-to-runtime assumptions in [ABI investigation](docs/abi.md) in an isolated test process, configure `-DONHIT_ENABLE_EXPERIMENTAL_HOOKS=ON` and set `BDS_ONHIT_ISOLATED_TEST=I_CONFIRM_ISOLATED_BDS` in that isolated process's environment. No production use is authorized. The optional detours are **unverified research code**; enabling them before ABI checks can crash the server or cause corruption. The three entrypoint hooks observe the vector at record `+0x120`, snapshot up to eight children, and preserve scalar RAX forwarding as hypothesized in the handoff. Events are buffered (32,768 maximum; overflow reported), then exported to `plugins/onhit_lifecycle/trace.tsv` when the isolated plugin is disabled. Do **not** hot reload/unload while hooks can be executing.

This initial trace **cannot prove child frees**: the deleting wrappers and allocator free path are not yet traced, pointer generation and alias tracking are offline analysis TODOs. It may distinguish reparse/reset branches but cannot alone support a patch.

## Rollback

Disable the entire plugin in the isolated server (`plugins/` removal while stopped); or rebuild without `ONHIT_ENABLE_EXPERIMENTAL_HOOKS`. No BDS memory is modified by the default passive build. For an active experimental build, **stop** the isolated server rather than hot unloading hooks during running calls.

## Project state

See [HANDOFF.md](HANDOFF.md) for exact verified state, [evidence](docs/evidence.md), [ABI gate](docs/abi.md), [test plan](docs/runtime-plan.md), [executed checks](docs/test-results.md) and Issues [#1](https://github.com/ReallocAll/bds-onhit-lifecycle/issues/1), [#2](https://github.com/ReallocAll/bds-onhit-lifecycle/issues/2), [#3](https://github.com/ReallocAll/bds-onhit-lifecycle/issues/3), [#4](https://github.com/ReallocAll/bds-onhit-lifecycle/issues/4).

Original investigation capsule: `IMPLEMENTATION_HANDOFF_PACKAGE.zip`, SHA-256 `9d7ab8b6f23948089ae05f8754d622f40c9dcfaead886281ef864d7b4612360f`. The capsule is not copied into this repository: it includes raw paths and broad investigative context, and it lacks the BDS binary and IDB. Recover it from the user-provided original artifact.
