# Isolated runtime acceptance plan (not executed)

1. **Select a zero-new-cost isolation path**, never production. Current SSH inventory lists `HOST_PROD` only. GitHub Actions billing/remaining free minutes could not be verified; **no workflow was triggered**. Avoid paid runners/cloud VMs.
2. Obtain an isolated copy of the *exact* 265,500,280-byte BDS ELF and Endstone v0.11.15; compare SHA-256 and GNU Build ID. Do not assume version string compatibility is enough.
3. Resolve the ABI/unwind issues in `abi.md`; then build and install diagnostic hooks only in the isolated test server. Run no-trigger start/stop and validate capture integrity, count, dropped/truncated events. Do not hot reload live detours.
4. Reproduce relevant entity/OnHit configuration; do not assume `ShulkerBullet` maps to the suspect path. Establish exact event/record mapping before attribution.
5. Record controlled repeated no-hit and hit cohorts after warm-up, with quiescent before/after jemalloc 5.4.0 profiles configured identically. Compare weighted `inuse_space`; keep `alloc_space`, RSS, samples and object pointers distinct. Record tick-time distributions, CPU, BDS version and compiler flags for overhead comparisons.
6. Add narrow deleting-wrapper/free-path probes where runtime evidence requires. Correlate `(child address, allocation generation)` and any shallow vector aliases. Verify actual free, not only wrapper entry or backing-buffer free. Account for missed traces and dropped events.
7. If an exclusive owner loss is shown, implement a reversible minimal fix in a **separate PR**, and compare baseline/hook/fix cohorts. Otherwise report no support for the presumed mechanism and redirect investigation.

### Acceptance ledger

| Gate | Status |
|---|---|
| Source scaffold | Implemented locally; experimental hooks unvalidated |
| Standalone identity negative gate | Local test passed |
| Bounded concurrent trace | Local test passed |
| Official Endstone C++ plugin build | Not run; SDK unavailable locally |
| Target ELF identity verified live | Not run |
| Hook ABI/unwind and isolated detour smoke | Not run |
| Exact child create/reset/release/alias trace | Not run |
| Leak confirmed or excluded | Neither |
| Patch and regression | Not attempted, appropriately gated |

No claim of low overhead may be made from unit tests. No production instrumentation, writes or traffic were performed.
