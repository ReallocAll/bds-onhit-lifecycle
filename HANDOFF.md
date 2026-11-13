# HANDOFF — BDS OnHit lifecycle investigation

Updated 2026-11-14 (Asia/Taipei). This public GitHub repository is the project state record. Supplement it with original `IMPLEMENTATION_HANDOFF_PACKAGE.zip` (SHA-256 `9d7ab8b6f23948089ae05f8754d622f40c9dcfaead886281ef864d7b4612360f`).

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
