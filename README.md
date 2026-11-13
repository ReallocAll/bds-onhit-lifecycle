# BDS OnHit Lifecycle — evidence-gated diagnosis

Private, **experimental** engineering workspace for BDS **1.26.60.6** / Endstone C++ API **v0.11**. The suspected `ProjectileComponent` / `OnHitSubcomponent` retention is **not a confirmed leak**.

## Safety and scope

- **Production BDS: read-only.** Never deploy this diagnostic or any runtime patch there.
- **No extra costs.** Do not run billable CI, paid infrastructure, or API services.
- **No speculative deletion.** Trace ownership and real deallocation before considering fixes.
- The original implementation handoff ZIP is retained by the user. Its SHA-256 is `9d7ab8b6f23948089ae05f8754d622f40c9dcfaead886281ef864d7b4612360f`. The ZIP does **not** contain the target BDS executable or IDB.

## Current phase

Initial repository is established; implementation and all runtime validation remain outstanding until independently verified. Use [HANDOFF.md](HANDOFF.md) and Issues [#1](https://github.com/ReallocAll/bds-onhit-lifecycle/issues/1), [#2](https://github.com/ReallocAll/bds-onhit-lifecycle/issues/2), [#3](https://github.com/ReallocAll/bds-onhit-lifecycle/issues/3), [#4](https://github.com/ReallocAll/bds-onhit-lifecycle/issues/4) for live state.

Only a BDS ELF with SHA-256 `ea16b541a448e07c337848d2c5f37321b706554b5216dd942f3f11e7a8ed2bad` and GNU Build ID `52576b75b6af5a09a6bde582be804e29657dbbf3` is eligible for the recorded IDA addresses.

## Evidence limits

The November 13, 2026 jemalloc 5.4.0 profiling data supersedes the older claim that production profiling was disabled; the reported retained-allocation sample counts are **not** live-object or retained-byte measurements. See upcoming `docs/evidence.md` for verified and outstanding evidence.
