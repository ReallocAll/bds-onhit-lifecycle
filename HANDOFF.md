# HANDOFF — BDS OnHit lifecycle investigation

Updated 2026-11-14 (Asia/Taipei). This private GitHub repository is the project state record. Supplement it with original `IMPLEMENTATION_HANDOFF_PACKAGE.zip` (SHA-256 `9d7ab8b6f23948089ae05f8754d622f40c9dcfaead886281ef864d7b4612360f`).

## Status

- **Repo**: `ReallocAll/bds-onhit-lifecycle`, private. `main` was initialized with a conservative README; implementation is in `feat/identity-and-diagnostic-scaffold` until PR approval/merge.
- **Issues**: #1 exact binary and ABI; #2 native diagnostics; #3 zero-cost isolated runtime and pointer/heap evidence; #4 conditional fix, never speculative.
- **Source**: CMake pinned Endstone v0.11.15; Linux executable SHA/Build ID/load-bias guard; default passive plugin; experimental compile-time gated three function detours; fixed 32768-slot capture buffer, overflow/truncation counters and quiescent TSV output.
- **Local checks**: C++ compiler/GCC, OpenSSL; CMake tests with `-DONHIT_BUILD_PLUGIN=OFF` passed (2/2: identity mismatch refusal and Build-ID extraction, multithread bounded trace). The same 2/2 passed with ASan+UBSan; plugin syntax checked only against temporary stubs, **not** Endstone. **Official Endstone plugin not built**; no BDS binary in the ZIP, no isolated BDS runtime, no target Hook validation.
- **Unresolved**: ABI/prologue/callsite and funchook unwind; runtime vptr/child identity, alias owners, allocations/frees; controlled weighted retained bytes; performance. Current experimental detours are a scaffold, not an approved runtime tool.
- **Leak/fix**: neither confirmed nor excluded; no fix implemented.

## Relevant known static facts

IDA image base 0; exact BDS SHA-256 `ea16b541a448e07c337848d2c5f37321b706554b5216dd942f3f11e7a8ed2bad`, GNU Build ID `52576b75b6af5a09a6bde582be804e29657dbbf3`. Record stride `0x2F8`, raw child vector at offsets `0x120/0x128/0x130`. Candidate functions `0x7143370`, `0x7143660`, `0x7757840`; only archived static caller-register evidence. Vector reset can lose references *conditionally*, while cleaner frees backing buffer; aliases and dealloc paths incomplete. See `docs/evidence.md` and `docs/abi.md`.

Production jemalloc **5.4.0 profiling was enabled as of 2026-11-13**, superseding the handoff's previous disabled claim. 69 `.heap` captures and count deltas were user-reported; profiles were not parsed in this task. Sample counts do not imply retained bytes.

## Immediate next action

1. Review this branch for source quality, especially the unknown ABI/prologue/unwind, before calling it usable. Do not merge unverified active-hook logic as a validated fix.
2. Obtain a **confirmed zero-cost** isolation path and the exact BDS executable, verify hash and Build ID, then close the hook safety gate.
3. Instrument actual deletes/frees and alias transitions as required by observations; correlate weighted jemalloc profiles and repeated control cohorts. Only then consider #4.

## Boundaries

Production server `HOST_PROD` is read-only; **never** deploy hooks, modify files, freeze/unfreeze/restart, or run new high-overhead profiles. SSH inventory found only production. GitHub 2026-11 usage summary was readable and reported zero billed Actions net cost to date, but no effective zero-overage budget/stop setting was verified. Do not start GitHub Actions unless zero extra billing risk is established. Do not use paid API/cloud resources. Use Evario J01/J02/J03/J04/J06/J07/J08 as decision gates. Maintain Issues, PRs, this HANDOFF and independently verified artifacts.

## 2026-11-14 verified production snapshot comparison (read-only)

Two already-captured profiles from PID 467575 (09:03 and 19:33 on 2026-11-13) were read via read-only SFTP and parsed per stack, using the official jemalloc `heap_v2` Poisson adjustment. In-use samples 1,606→2,125; stacks containing raw main-image return PC EA `0x71439E0` 719→1,182. **Estimated weighted** `inuse_space` all paths 1,702.573→2,220.286 MiB (+517.713 MiB); OnHit-associated stack 719.036→1,182.060 MiB (+463.024 MiB), **89.4% of weighted difference**. This is stronger stack attribution, **not object-level leak proof**. No new production samples or code changes were made. Full method and raw-vs-weighted cautions: `docs/heap-profile-observations.md`; offline parser: `tools/heap_v2_summary.py`; test: `python3 tests/test_heap_v2.py`.

Official Endstone v0.11.15 source review confirms Clang+Ninja and Linux libc++/libc++abi build requirements. Local container has clang and Ninja but lacks libc++ headers; no official SDK build or isolated BDS Hook acceptance occurred. Continue #1/#2/#3. Experimental source remains **draft** and should not be activated on production.
