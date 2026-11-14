# OnHitFix (Linux preview)

OnHitFix is a minimal Endstone native plugin for **BDS 1.26.60.6 Linux x86-64** using **Endstone 0.11.15**. It applies the experimental `CLEANUP_RECLAIM` repair automatically on plugin load. **Not production-certified.**

## Install / rollback

Download `endstone_onhitfix.so` from the latest successful [build artifact](https://github.com/ReallocAll/bds-onhit-lifecycle/actions/workflows/native-build.yml), stop BDS, copy the library into `plugins/`, and start the server. No configuration or environment variable is required.

To roll back, **stop the server**, remove `plugins/endstone_onhitfix.so`, then restart. Do not hot-unload while the hook might be executing.

The plugin installs its single Cleanup hook only when the **running BDS executable** matches SHA-256 `ea16b541a448e07c337848d2c5f37321b706554b5216dd942f3f11e7a8ed2bad`, GNU Build ID `52576b75b6af5a09a6bde582be804e29657dbbf3`, and executable segment checks. No hook is installed on a mismatch.

## Commands (operators and console)

- `/ohf status` — `Obj del` is the cumulative number of BDS child deleting-destructor calls made by this plugin since process start. `Obj mem` is the cumulative **estimated object capacity processed**, not exact leaked bytes or RSS reduction. Only independently verified factory object sizes are counted; unknown sizes contribute zero.
- `/ohf hit` — up to 20 hottest chunks by Endstone `ActorDamageEvent` with `DamageSource.getType() == "projectile"`, using the damaged actor's dimension and X/Z chunk-start block coordinates. A fixed-capacity 128-entry Space-Saving sketch keeps bounded memory; counts and ranking become **approximate** after saturation. These events are not exact Cleanup or leak attribution.

Statistics are process-local and reset on restart.

## Known risk

**Preview only:** BDS `0x7757F60` can shallow-copy child pointer values across records. Whether two live records can both reclaim the same allocation generation remains **INCONCLUSIVE**. **Double-free and use-after-free have not been excluded.** Prior isolated Shulker tests showed target retained-allocation improvement without observed crashes, but they do not establish ownership safety or production stability. No production installation is recommended without accepting this risk.

Engineering evidence and run history: [HANDOFF.md](HANDOFF.md).
