# ABI and Hook safety gate — not cleared

## Static hypotheses for exact BDS ELF

| Candidate | IDA EA | Raw input registers | Reported return | Critical uncertainty |
|---|---:|---|---|---|
| record initializer | `0x7143370` | RDI | RAX/int64 | callsite and exception behavior |
| on-hit parse/factory | `0x7143660` | RDI, RSI | RAX/uint64 | RSI semantic role unknown |
| cleanup/move | `0x7757840` | RDI, RSI, RDX, RCX | RAX/int64 | opaque extra arguments and tail jumps |

These shapes are reconstructed from IDA, not C++ source-level ABI contracts. No target BDS binary is available for fresh `objdump`/`readelf` verification in this environment.

Relevant callsites: `0xB38FE01` (parse), `0x7757F05` (reset), `0x7756A0E`, `0x7756A24`, `0x7756AD7`, `0x7756B98`, `0x7758522`, and tail-jump `0x77569D0` (cleanup). Before installing a detour, inspect each prologue and complete callsite, hidden/aggregate/FPU arguments and returns, stack alignment, volatile register effects and exception propagation. Remember that a tail jump can leave a different unwind chain.

## Non-negotiable gate

1. Verify `/proc/self/exe` SHA-256 and **loaded main ELF** GNU Build ID. Both must match exactly. Verify EA+load-bias is within an executable mapped segment.
2. Verify target entry bytes and all caller conventions against the actual same ELF. The current identity gate implements only step 1; **the stronger ABI gate is not implemented**.
3. Determine how funchook 1.1.3 relocates prologues and whether a C++ exception can unwind through its trampoline. Do not enable the three C++ detours unless this is established (or replace them with a proven passive/assembly probe). `noexcept` event writers may not suppress/convert target exceptions.
4. Run an isolated no-trigger startup/shutdown, Hook install/uninstall and real hit/no-hit experiment. Validate event capture and BDS stability before interpreting any pointer.
5. Match deletion by pointer **and allocation generation**; track copies/aliases and concrete free calls. A `+8` wrapper *entry* alone is not a confirmed object deallocation.

## Current code limitation

`src/plugin.cpp` contains compile-time disabled experimental detours to support later experiments without modifying the Endstone core. They are **not enabled by default**, **not ABI-verified**, and **not tested against the target**. They collect pointer snapshots but do not observe object allocations or actual frees. They must not be used as-is to justify a deletion fix.

## Design evidence

Evario Endstone #420 case (API/ABI/runtime proof separation); MT01 ownership test scope; PT01 observer cost/loss accounting. Bounded source: archived `key-disassembly.md` and `IMPLEMENTATION_HANDOFF.md`.
