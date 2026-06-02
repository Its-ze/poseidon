# POSEIDON v0.6.1 — Full Audit & Refactor Design

**Date:** 2026-06-01
**Status:** Design — approved, awaiting spec review
**Author:** Claude (Opus 4.7) with user
**Scope:** Core POSEIDON (`src/`, `src/features/`) + `c5_node/`
**Reference firmwares:** Bruce (bmorcelli), Ghost ESP (Spooks4576), PORKCHOP (0ct0sec), Evil-M5Project (7h30th3r0n3)

---

## 1. Goal

Produce a complete audit of POSEIDON at v0.6.1, cross-referenced against four sibling firmwares for the M5Stack Cardputer-Adv, and deliver a phased refactor plan that identifies optimisations, capability gaps, and the work required to close them.

The audit is paper-only — no hardware-in-loop, no library-internal disassembly, no live UI testing. Those are explicit out-of-scope risks logged in §7.

## 2. Deliverables

Four markdown files written to `docs/audit/`:

| File | Purpose |
|------|---------|
| `AUDIT.md` | Executive summary · per-module scorecard (A–F across memory / hw / perf / quality) · top-20 defects with `file:line` |
| `CROSS_REF.md` | Feature matrix — rows: features; columns: POSEIDON / Bruce / Ghost / Porkchop / EvilM5; cells: verdict + `file:line`. Gaps + leads called out. |
| `REFACTOR_PLAN.md` | Phase 0 (hygiene) → Phase 1 (hardware safety) → Phase 2 (module splits) → Phase 3 (feature parity) → Phase 4 (new capabilities). Sequencing, risk, exit criteria per phase. |
| `BACKLOG.md` | Discrete tickets — ID · title · severity · module · `file:line` · fix recipe · est effort. Sorted by severity then phase. |

## 3. Architecture — Three-wave agent pipeline

```
Wave 1 (parallel × 5)  →  Wave 2 (parallel × 6)  →  Wave 3 (sequential × 1)
   inventory + ref maps      module bucket audits      synthesis + 4 docs
   ↓                          ↓                          ↓
_audit/maps/               _audit/findings/          docs/audit/
```

Working directory: `C:\Users\D\poseidon\_audit\` — gitignored, intermediate artifacts only.

### Wave 1 — Inventory & reference mapping (5 parallel agents)

- **`poseidon-mapper`** — walks the in-scope source, emits `_audit/maps/poseidon.md`: per-file 1-line purpose, LOC, public API surface, gotcha tags from memory (`TX-cache`, `WiFi-AP-IDF`, `BLE-coop`, `IR-active-low`, `GPS-off`, `HSPI-park`).
- **`bruce-mapper`** — shallow clone `bmorcelli/Bruce` → `_audit/refs/bruce/`, emit `_audit/maps/bruce.md`: per-feature index keyed to POSEIDON's feature taxonomy (`feature → file:line`).
- **`ghost-mapper`** — same for `Spooks4576/Ghost_ESP`.
- **`porkchop-mapper`** — same for `0ct0sec/M5PORKCHOP`.
- **`evilm5-mapper`** — same for `7h30th3r0n3/Evil-M5Project`.

Each reference map is capped at ≤200 lines and structured as a lookup index, not a tour.

### Wave 2 — Module bucket auditors (6 parallel agents)

Each auditor receives all 5 Wave-1 maps in its context. Output: `_audit/findings/<bucket>.md` with sections — *Current state · Defects · Cross-ref deltas (per reference firmware) · Optimisation opportunities · Refactor targets · Backlog items.* Cap ≤500 lines.

| # | Bucket | File pattern |
|---|--------|-------------|
| 1 | **WiFi** | `wifi_*.{cpp,h}`, `evil_twin*`, `wifi_sanity_override`, `wifi_wardrive*` |
| 2 | **BLE** | `ble_*.{cpp,h}`, `ble_db`, `ble_types` |
| 3 | **Sub-GHz + RF radios** | `subghz_*`, `cc1101_*`, `radio*`, `lora_*`, `rf_finder`, `nrf24*`, `nrf52*` |
| 4 | **Net attacks + comms** | `net_*`, `dhcp_cache`, `satcom*`, `mesh*`, `c5_cmd` |
| 5 | **System / UI / c5_node** | `main`, `app.h`, `menu*`, `ui*`, `theme*`, `input`, `sfx*`, `splash`, `screensaver*`, `hat_manager`, `c5_node/*` |
| 6 | **IR + GPS + specials** | `ir_*`, `gps`, `drone_remoteid`, `surveillance*`, `badusb*`, `mimir*`, `trident*`, `triton*`, `defensive_monitor`, `saltyjack/*` |

### Wave 3 — Synthesis (1 sequential agent)

Reads every `_audit/*` artifact, writes the 4 deliverables in §2. No new analysis — only aggregation, deduplication, prioritisation, formatting.

## 4. Audit criteria

Every Wave-2 auditor checks the following, in order:

- **Memory** — heap/stack budgets; PSRAM-disabled safety (this unit's PSRAM is broken — no code path may assume PSRAM exists); allocator hotspots; leak paths.
- **Hardware** — SPI bus ownership (FSPI display vs HSPI SD/nRF24/CC1101); pin conflicts vs the GPIO map in `platformio.ini`; IR LED polarity (active-low, GPIO 44); ESP-NOW channel locking.
- **Concurrency** — cooperative-tick compliance for BLE (NimBLE init eats heap → `xTaskCreate` fails silently with `rc=-1`); NimBLE init order; ISR safety.
- **Performance** — TX layout sensitivity (`pushImage` during WiFi TX causes cache stall sprite scramble; cache to SRAM); refresh budget; deauth frame rate.
- **OPSEC** — GPS-tag defaults (must be OFF unless user enables); capture file handling; BLE/WiFi MAC randomisation; SD write paths.
- **Build** — lib pinning drift vs Bruce 20260123; partition layout vs 8MB flash; `-zmuldefs` validity for the sanity override.
- **Code quality** — files >800 LOC flagged; duplication; dead code / stubs; gate consistency (hat-present checks, hardware-init guards).
- **Cross-ref delta** — per reference firmware, classify each feature as one of: `we_lack` · `we_have_better` · `parity` · `divergent_approach`.

## 5. Constraints encoded into auditor prompts

Pulled from project memory; auditors flag drift as defects rather than suggestions:

- POSEIDON release gate — no push without explicit user approval per release; backlog tickets requiring a release tag are marked `user-gated`.
- Token discipline — Wave 2 auditors capped at ≤500 lines; Wave 3 docs scale to content but stay scannable.
- One-fix-at-a-time on hardware-in-loop — backlog tickets atomic, not bundled.
- GPS-off default, BLE cooperative tick, IR active-low, WiFi raw-IDF AP recipe — invariants, not preferences.
- Bruce lib pinning at `20260123` — any audit-driven bump must justify the regression risk against the previous `20260407` failure (rc=258 on-device).

## 6. Acceptance criteria

The audit pass is complete when:

- All 4 deliverables exist at `docs/audit/<name>.md` and are internally consistent (no claim in `AUDIT.md` lacks a backlog ticket; no `CROSS_REF.md` cell lacks a `file:line` citation where one is expected).
- Every defect referenced in `AUDIT.md` resolves to a real `file:line` in the current checkout (synthesis agent verifies).
- `REFACTOR_PLAN.md` Phase 0–4 each lists exit criteria, not just task lists.
- `BACKLOG.md` ticket count balances: every Wave-2 finding produces ≥1 ticket; no orphan tickets.
- The 4 docs together do not exceed ~3000 lines combined — if they do, synthesis re-scopes rather than truncates.

## 7. Out of scope / known blind spots

- **Hardware-in-loop validation** — paper audit only. Tickets may need a live verification pass before close.
- **Library-internal issues** — auditors will not read Bruce-libs binary archive (`framework-arduinoespressif32-libs`). Anything beneath the lib boundary surfaces as "suspected upstream" only.
- **UX flow / menu navigation** — static read only. Dynamic UI issues (e.g. menu carousel jitter, screensaver wake races) will surface only if visible in source.
- **Satellite firmwares** — `argus_node/`, `proteus_node/`, `siren_node/` are explicitly excluded this pass. Audit `c5_node/` only.
- **Runtime perf measurement** — no flash/profile. Performance findings are static analysis (allocation patterns, frame layout, refresh paths).

## 8. Sequencing & checkpoints

1. Write & approve this spec (current step).
2. Invoke `superpowers:writing-plans` to produce the implementation plan covering Wave 1 / Wave 2 / Wave 3 dispatch, artifact paths, prompt templates per agent, and verification gates.
3. Execute Wave 1 — 5 parallel agents.
4. Verification gate — user reviews `_audit/maps/*.md` before Wave 2 dispatches.
5. Execute Wave 2 — 6 parallel agents.
6. Verification gate — user reviews `_audit/findings/*.md` before Wave 3 dispatches.
7. Execute Wave 3 — synthesis.
8. Final review — user reads the 4 deliverables in `docs/audit/`. Refactor work begins from `BACKLOG.md` in subsequent sessions.

## 9. Out-of-design decisions deferred to plan

- Exact agent subagent_type per wave (Explore vs general-purpose vs feature-dev:code-explorer).
- Whether to use `--depth=1` or `--filter=blob:none` for reference clones.
- `.gitignore` entries for `_audit/`.
- Whether `BACKLOG.md` ticket IDs use a prefix scheme (`POS-AUDIT-001`) or flat numbering.
