# HANDOFF — BDS OnHit lifecycle investigation

Updated 2026-11-14 (Asia/Taipei). This public GitHub repository is the project state record. Supplement it with original `IMPLEMENTATION_HANDOFF_PACKAGE.zip` (SHA-256 `9d7ab8b6f23948089ae05f8754d622f40c9dcfaead886281ef864d7b4612360f`).

## Current verified checkpoint (2026-11-14 04:51 Asia/Taipei)

This section supersedes earlier chronological statements below that say the official SDK, passive BDS load, or unattended Shulker bullet environment have not been tested. Historical entries are retained as an audit trail.

- OnHit development head before this checkpoint: `f138df7c591bc5d91b00d9a678229443c619fdd9`; baseline `main`: `6f00c63968ba9a8192073b47bbf7ec096c32cfd1`. Draft PR [#5](https://github.com/ReallocAll/bds-onhit-lifecycle/pull/5) remains open.
- Testbed `experiment/onhit-shulker`: `b4f42d3a3890972a67ef352d73b3a59d3babe725`, [draft PR #29](https://github.com/ReallocAll/bedrock-testbed/pull/29); Testbed `main`: `69ff87eac8d05a0c1da4ccb387005d32012e0003` (unchanged). Bot `main`: `4bd0691f1ef0f551e5ba258d4747b35a3b748177`.
- Official SDK build [43486566882](https://github.com/ReallocAll/bds-onhit-lifecycle/actions/runs/43486566882), isolated passive BDS [43487320217](https://github.com/ReallocAll/bedrock-testbed/actions/runs/43487320217), and unattended actual `shulker_bullet` observation [43490505011](https://github.com/ReallocAll/bedrock-testbed/actions/runs/43490505011) are **passed**. The Testbed still pins OnHit `7e8940dd99a7a209d3fec8a602a784ab67a4b6a2`; later documentation commits are not separately runtime-tested.
- **Critical unverified gate:** `docs/abi.md` target entry bytes/callsites/argument and return ABI, funchook trampoline unwind, detour uninstall quiescence. `src/plugin.cpp::emit` currently dereferences a hypothesized record and vector with `memcpy`; checking vector ordering does not prove address readability, lifetime, or thread safety. Never activate these experimental detours on the strength of the passive tests. Also prove an actual projectile **contact**, not just bullet existence.
- No allocation-generation/alias/free evidence, matched hit/no-hit retained-memory cohorts, object-level leak proof, or remediation exists. Do not merge either draft or disable the recurring investigation. No new Actions run was triggered for this checkpoint.

## Status

- **Repo**: `ReallocAll/bds-onhit-lifecycle`, public. `main` was initialized with a conservative README; implementation is in `feat/identity-and-diagnostic-scaffold` until PR approval/merge.
- **Issues**: #1 exact binary and ABI; #2 native diagnostics; #3 zero-cost isolated runtime and pointer/heap evidence; #4 conditional fix, never speculative.
- **Source**: CMake pinned Endstone v0.11.15; Linux executable SHA/Build ID/load-bias guard; default passive plugin; experimental compile-time gated three function detours; fixed 32768-slot capture buffer, overflow/truncation counters and quiescent TSV output.
- **Local checks**: C++ compiler/GCC, OpenSSL; CMake tests with `-DONHIT_BUILD_PLUGIN=OFF` passed (2/2: identity mismatch refusal and Build-ID extraction, multithread bounded trace). The same 2/2 passed with ASan+UBSan; plugin syntax checked only against temporary stubs, **not** Endstone. **Official Endstone v0.11.15 plugin now builds on standard Ubuntu 24.04 with Clang 18/libc++** (CI run 43486566882 at `7e8940dd99a7a209d3fec8a602a784ab67a4b6a2`, 2/2 CTest and heap parser passed); no isolated BDS runtime acceptance or target Hook validation yet.
- **Unresolved**: ABI/prologue/callsite and funchook unwind; runtime vptr/child identity, alias owners, allocations/frees; controlled weighted retained bytes; performance. Current experimental detours are a scaffold, not an approved runtime tool.
- **Leak/fix**: neither confirmed nor excluded; no fix implemented.

## Relevant known static facts

IDA image base 0; exact BDS SHA-256 `ea16b541a448e07c337848d2c5f37321b706554b5216dd942f3f11e7a8ed2bad`, GNU Build ID `52576b75b6af5a09a6bde582be804e29657dbbf3`. Record stride `0x2F8`, raw child vector at offsets `0x120/0x128/0x130`. Candidate functions `0x7143370`, `0x7143660`, `0x7757840`; only archived static caller-register evidence. Vector reset can lose references *conditionally*, while cleaner frees backing buffer; aliases and dealloc paths incomplete. See `docs/evidence.md` and `docs/abi.md`.

Production jemalloc **5.4.0 profiling was enabled as of 2026-11-13**, superseding the handoff's previous disabled claim. 69 `.heap` captures and count deltas were user-reported; profiles were not parsed in this task. Sample counts do not imply retained bytes.

## Immediate next action

1. Official SDK C++ compilation/linking is now proven, but review unknown ABI/prologue/unwind before any active detour; do not merge a diagnostic scaffold as a validated fix.
2. Continue Testbed draft PR #29 on `experiment/onhit-shulker` (separate from main), in standard free public Ubuntu Actions; verify real BDS executable SHA/Build ID, passive plugin load and normal shutdown, then resolve ABI before active detours.
3. Instrument actual deletes/frees and alias transitions as required by observations; correlate weighted jemalloc profiles and repeated control cohorts. Only then consider #4.

## Boundaries

Production server `HOST_PROD` is read-only; **never** deploy hooks, modify files, freeze/unfreeze/restart, or run new high-overhead profiles. SSH inventory found only production. For public repos, GitHub's current billing and runner docs establish standard `ubuntu-24.04` GitHub-hosted runner compute is free. We use only that runner, without larger runners or artifact uploads. Private-repo overage remains unauthorized. Do not use paid API/cloud resources. Use Evario J01/J02/J03/J04/J06/J07/J08 as decision gates. Maintain Issues, PRs, this HANDOFF and independently verified artifacts.

## 2026-11-14 verified production snapshot comparison (read-only)

Two already-captured profiles from PID 467575 (09:03 and 19:33 on 2026-11-13) were read via read-only SFTP and parsed per stack, using the official jemalloc `heap_v2` Poisson adjustment. In-use samples 1,606→2,125; stacks containing raw main-image return PC EA `0x71439E0` 719→1,182. **Estimated weighted** `inuse_space` all paths 1,702.573→2,220.286 MiB (+517.713 MiB); OnHit-associated stack 719.036→1,182.060 MiB (+463.024 MiB), **89.4% of weighted difference**. This is stronger stack attribution, **not object-level leak proof**. No new production samples or code changes were made. Full method and raw-vs-weighted cautions: `docs/heap-profile-observations.md`; offline parser: `tools/heap_v2_summary.py`; test: `python3 tests/test_heap_v2.py`.

Official Endstone v0.11.15 source review confirms Clang+Ninja and Linux libc++/libc++abi build requirements. Local container has clang and Ninja but lacks libc++ headers; no official SDK build or isolated BDS Hook acceptance occurred. Continue #1/#2/#3. Experimental source remains **draft** and should not be activated on production.

## 2026-11-14 official SDK and Testbed progress (verified GitHub evidence)

- Official Endstone v0.11.15 native C++ build was first attempted in public standard Actions at commit `603d6f9e760583f813017732eec4e3fb6fdb4d03`; it **failed**, revealing `Plugin final` was incompatible with the real `ENDSTONE_PLUGIN` macro (macro subclasses the plugin class). The earlier handwritten stub could not detect this.
- Fixed only that declaration at commit `7e8940dd99a7a209d3fec8a602a784ab67a4b6a2`. GitHub Actions run [43486566882](https://github.com/ReallocAll/bds-onhit-lifecycle/actions/runs/43486566882) **succeeded**: Clang 18/libc++ official SDK full .so link, 2/2 CTest, Python heap-profile test, ELF check. This verifies compilation/linking, **not** BDS load, Hook ABI or lifecycle.
- CI branch workflow was narrowed to one push trigger to avoid duplicate push+PR executions at commit `df6696a260f094559170954aaee43b060303ee94`.
- Testbed is now public and has independent experimental branch `experiment/onhit-shulker`, draft [PR #29](https://github.com/ReallocAll/bedrock-testbed/pull/29). It uses a pinned exact OnHit source commit `7e8940dd99a7a209d3fec8a602a784ab67a4b6a2`, Endstone 0.11.15, BDS 1.26.60.6, hooks disabled, no production access or artifacts. The first BDS workflow run [43487065684](https://github.com/ReallocAll/bedrock-testbed/actions/runs/43487065684) built the plugin, then failed before BDS launch because a new Python constructor signature included a literal backslash-n. Fixed by Testbed commit `1fd3f98c29f44716982f242adecbc396b44d0fb5`; follow the subsequent CI run for actual runtime acceptance.
- **Still unverified**: exact BDS ELF match on the testbed, native plugin load and shutdown, target ABI/unwind, dynamic OnHit trigger, child ownership/free, matched retained-byte controls and any fix. Keep draft PRs; do not enable experimental hooks until ABI gate clears.

## 2026-11-14 isolated real BDS passive acceptance — PASS

[bedrock-testbed run 43487320217](https://github.com/ReallocAll/bedrock-testbed/actions/runs/43487320217) on `experiment/onhit-shulker` commit `1fd3f98c29f44716982f242adecbc396b44d0fb5` **passed**. Fresh standard Ubuntu 24.04 Runner installed Endstone 0.11.15, built pinned OnHit `7e8940dd99a7a209d3fec8a602a784ab67a4b6a2` against official SDK, bootstrapped isolated BDS 1.26.60.6, verified SHA-256 `ea16b541a448e07c337848d2c5f37321b706554b5216dd942f3f11e7a8ed2bad`, GNU Build ID `52576b75b6af5a09a6bde582be804e29657dbbf3` and ELF byte length `265500280`. The same isolated world was retained between bootstrap and native-plugin startup. The logs show:
- `Loading onhit_lifecycle v0.1.0`, `Exact BDS image verified`, `Enabling onhit_lifecycle v0.1.0`
- `Diagnostics passive only; hooks were excluded at build time.`
- `Disabling onhit_lifecycle v0.1.0`, graceful stop, no residual BDS processes

This **closes the passive load/integrity smoke gate** (Stage B) and confirms a zero-additional-cost actual BDS execution path. It does **not** establish detour ABI/unwind or any OnHit leak. Continue with Endstone-bot Beta API two-start initialization and autonomous Shulker projectile trigger in the Testbed experiment branch; still prohibit active hooks until the target ABI, prologue and unwind gates are cleared.

## 2026-11-14 fully unattended Shulker projectile environment — PASS

[bedrock-testbed run 43490505011](https://github.com/ReallocAll/bedrock-testbed/actions/runs/43490505011) on `experiment/onhit-shulker` commit `b4f42d3a3890972a67ef352d73b3a59d3babe725` **passed** the complete headless environment setup and **observed an actual vanilla `shulker_bullet`** inside isolated BDS 1.26.60.6. OnHit native plugin pinned to `7e8940dd99a7a209d3fec8a602a784ab67a4b6a2`; bot plugin pinned to `4bd0691f1ef0f551e5ba258d4747b35a3b748177`.

- Bootstrap / initial bot enable: installs EndstoneBot Behavior Pack 4.3.3 and writes `plugins/bot/enable_beta.py`.
- Gracefully stop BDS; execute the emitted Beta APIs patch on the *same* world only while BDS stopped. Patch reports `Beta APIs enabled successfully`.
- Start BDS again: logs confirm `Experiment(s) active: gtst`, Bridge protocol 3 authenticated and `standalone SimulatedPlayer 功能可用`.
- Difficulty Normal. Console `bot createat` spawns a Survival SimulatedPlayer `OnHitTarget` and BDS player list independently reports that name online.
- Wait for generated chunk after fake player joins, then `fill` reports **169 blocks filled**, bot `moveat` succeeds, **three** `summon shulker` commands report `Object successfully summoned`.
- Third server-side entity selector check executes `say ONHIT_BULLET_OBSERVED`; console shows `[] ONHIT_BULLET_OBSERVED` and JSON reports `shulker_bullet_observed: true`, `bullet_queries: 3`.
- Both passive plugin and bot stop cleanly. Output is preserved in the public GitHub Actions run logs; no world, BDS binary or sensitive artifacts uploaded. This is repeatable and does **not** require a human client.

**Meaning:** Stage B and the *Shulker projectile availability* part of Stage C are proven. **Not proven**: the projectile made contact with a target; the specific OnHit factory path fired; any native detour is ABI/unwind safe; lifecycle/alias/free events; a leak; or any fix. Hook remains compile-time OFF. The next highest-value task is exact ELF prologue/callsite/ABI verification, then isolated active Hook with a real hit oracle and lifetime tracing. Keep both Draft PRs open until appropriate proof.

## 2026-11-14 05:51 checkpoint — scalar-only hook capture

- At OnHit commit `8498b59b7aeda45019586b69ad39bb5ab379f892`, removed the speculative `record+0x120` and child-array `memcpy` operations from `src/plugin.cpp::emit`. If experimental detours are eventually enabled, their event capture now records only scalar arguments, return values, caller, timestamp and thread ID. Existing vector/child TSV columns remain but are zero/invalid; **do not interpret them as observed empty vectors**.
- This removes one unsafe dereference path, **not** the ABI/prologue/trampoline/unwind/uninstall gate. Hook activation is still prohibited pending independent exact-ELF proof. It also does not establish a lifetime or leak diagnosis.
- Previous descriptions of active snapshot collection refer to historical source. No new CI or isolated BDS run is claimed for this commit. Testbed continues to pin `7e8940dd99a7a209d3fec8a602a784ab67a4b6a2`; neither Testbed main nor its experiment branch was changed this round.
- Next: obtain bounded exact-ELF prologue/callsite disassembly and funchook unwind/quiescence proof, then add ownership-aware observation only after validated memory-access and concurrency assumptions. Do not reintroduce unchecked `memcpy` merely to populate legacy TSV fields.

## 2026-11-14 exact-ELF ABI interpretation checkpoint

At OnHit commit `04586083433e5d8621531cc4c1844fc7f34ab16d`, `docs/abi.md` records a new review of the **existing** successful [Testbed run 43529829494](https://github.com/ReallocAll/bedrock-testbed/actions/runs/43529829494) job `132244476886`. The exact ELF demonstrates parse reads `rsi` and writes through `rdi`; reset's observed caller overwrites `rax` immediately after the call; cleanup's first instructions use `rdi`, with a confirmed tail-jump caller. Entry whole-instruction spans are 5/5/11 bytes (reset/parse/cleanup), **not** proven funchook patch or trampoline sizes. The existing `ResetFn` integer return and four-argument `CleanupFn` are not verified signatures. The exporter window's trailing partial instructions must not be mistaken for invalid target code.

**Acceptance:** static analysis/documentation only; no new CI or BDS run, no detour activation, no proof of contact, object free, leak, or fix. Retain both Draft PRs, keep Testbed main unchanged, and do not stop the recurring task. Next: validate native ABI and funchook relocation/unwind/uninstall before an isolated minimal active-hook probe.

## 2026-11-14 11:44 Asia/Taipei — callsite validator execution gate

- Live heads checked: OnHit main `6f00c63968ba9a8192073b47bbf7ec096c32cfd1`; Testbed main `69ff87eac8d05a0c1da4ccb387005d32012e0003`; bot main `4bd0691f1ef0f551e5ba258d4747b35a3b748177`. Both draft PRs #5/#29 remain open; OnHit issues #1–#4 open.
- Found a concrete failure in `tools/export_abi_evidence.py`: raw Python regex literals used doubled backslashes (`\\\\s` / `\\\\b`) for whitespace/word boundary. These match literal backslash sequences instead of GNU objdump whitespace, so the new strict callsite verifier would refuse valid disassembly. Corrected in OnHit commit [03cf765](https://github.com/ReallocAll/bds-onhit-lifecycle/commit/03cf765d2c68848561c809ac184c4d6192bb0904). The earlier Testbed successful run pinned an older exporter and **did not execute this new strict validator**.
- Testbed experiment workflow was updated to pin the exact corrected OnHit SHA in commit [f65345b](https://github.com/ReallocAll/bedrock-testbed/commit/f65345b129d0b379fa0d3e6a90d19a2bbc8b6185). Public standard `ubuntu-24.04` runner only; Testbed main unchanged. At the time of writing the new Testbed run was not yet listed, so do not claim the validator or runtime passed. OnHit build run [43553177969](https://github.com/ReallocAll/bds-onhit-lifecycle/actions/runs/43553177969) was in progress.
- Next: inspect the new Testbed run and actual `Verified call/jmp` output for all eight callsites; if failing, fix the parser against real objdump formatting. This still does **not** approve C++ ABI, funchook trampoline/unwind, thread quiescence or active detours. No Hook hit, allocation/free evidence, leak proof or fix yet.
