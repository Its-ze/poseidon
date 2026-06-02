# POSEIDON v0.6.1 Full Audit — Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use `superpowers:subagent-driven-development` (recommended) or `superpowers:executing-plans` to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Run the 3-wave audit pipeline defined in `docs/superpowers/specs/2026-06-01-poseidon-full-audit-design.md` and produce the four deliverables under `docs/audit/`.

**Architecture:** 5 parallel Wave-1 mappers → 6 parallel Wave-2 module auditors → 1 sequential Wave-3 synthesis. Intermediate artifacts in `_audit/`, final deliverables in `docs/audit/`. All work on branch `audit/v0.6.1` — no remote pushes (POSEIDON release gate).

**Tech Stack:** Claude agent dispatch (Agent tool with `general-purpose` and `feature-dev:code-explorer`), bash for git/clone, markdown artifacts.

---

## Pre-flight: Branch + scaffolding

### Task 0: Create working branch and audit dirs

**Files:**
- Create: `_audit/.gitkeep`
- Create: `docs/audit/.gitkeep`
- Modify: `.gitignore` (append `_audit/refs/` and `_audit/_capture.log`)

- [ ] **Step 1: Create audit branch from master**

```bash
cd /c/Users/D/poseidon
git checkout -b audit/v0.6.1
```

Expected: `Switched to a new branch 'audit/v0.6.1'`

- [ ] **Step 2: Create artifact dirs**

```bash
mkdir -p _audit/maps _audit/refs _audit/findings docs/audit
touch _audit/.gitkeep docs/audit/.gitkeep
```

- [ ] **Step 3: Update .gitignore — keep maps/findings, ignore cloned refs**

Append to `.gitignore`:

```
# Audit pipeline — keep maps/findings, ignore upstream clones and logs
_audit/refs/
_audit/_capture.log
```

- [ ] **Step 4: Commit scaffolding**

```bash
git add .gitignore _audit/.gitkeep docs/audit/.gitkeep docs/superpowers/specs/2026-06-01-poseidon-full-audit-design.md docs/superpowers/plans/2026-06-01-poseidon-full-audit.md
git commit -m "audit: scaffold v0.6.1 full audit branch (spec + plan + dirs)"
```

Expected: commit succeeds, working tree clean except for any unrelated changes.

---

## Wave 1 — Inventory + reference mapping

Five agents dispatched in ONE message (parallel). Each writes a single markdown artifact.

### Task 1.1: Dispatch all 5 Wave-1 agents in parallel

**Files (produced):**
- Create: `_audit/maps/poseidon.md`
- Create: `_audit/maps/bruce.md`
- Create: `_audit/maps/ghost.md`
- Create: `_audit/maps/porkchop.md`
- Create: `_audit/maps/evilm5.md`
- Create (transient): `_audit/refs/{bruce,ghost,porkchop,evilm5}/` (shallow clones, gitignored)

- [ ] **Step 1: Send a single message with 5 Agent tool calls (see prompts in Appendix A)**

Agent calls in one message:
- A1.1 `poseidon-mapper` — `subagent_type: feature-dev:code-explorer`
- A1.2 `bruce-mapper` — `subagent_type: general-purpose`
- A1.3 `ghost-mapper` — `subagent_type: general-purpose`
- A1.4 `porkchop-mapper` — `subagent_type: general-purpose`
- A1.5 `evilm5-mapper` — `subagent_type: general-purpose`

- [ ] **Step 2: Verify all 5 artifacts exist**

```bash
ls -la _audit/maps/
```

Expected: 5 files, each non-empty (`poseidon.md`, `bruce.md`, `ghost.md`, `porkchop.md`, `evilm5.md`).

- [ ] **Step 3: Verify each map is ≤200 lines (reference maps) / unbounded for POSEIDON map**

```bash
wc -l _audit/maps/*.md
```

Expected: `bruce.md`, `ghost.md`, `porkchop.md`, `evilm5.md` each ≤ ~220 lines (200 cap + frontmatter slack). `poseidon.md` may be larger.

- [ ] **Step 4: Smoke-check map structure — every map must contain a "feature → file:line" lookup table**

```bash
grep -l "file:line\|file:" _audit/maps/*.md
```

Expected: all 5 files match.

### Task 1.2: Wave-1 review gate

- [ ] **Step 1: Present `_audit/maps/poseidon.md` summary to user (top-level file groupings + gotcha tag distribution)**

- [ ] **Step 2: Present each reference map's feature index header (first 30 lines) to user**

- [ ] **Step 3: User signals "go for wave 2" or requests map fixes**

If fixes requested: re-dispatch the affected mapper with corrections, then re-verify.

### Task 1.3: Commit Wave-1 artifacts

- [ ] **Step 1: Stage and commit maps**

```bash
cd /c/Users/D/poseidon
git add _audit/maps/
git commit -m "audit: wave 1 — POSEIDON inventory + Bruce/Ghost/Porkchop/EvilM5 reference maps"
```

Expected: commit succeeds. `_audit/refs/` stays unstaged (gitignored).

---

## Wave 2 — Module auditors

Six auditors dispatched in ONE message (parallel). Each receives all 5 Wave-1 maps in its prompt.

### Task 2.1: Dispatch all 6 module auditors in parallel

**Files (produced):**
- Create: `_audit/findings/01_wifi.md`
- Create: `_audit/findings/02_ble.md`
- Create: `_audit/findings/03_subghz_rf.md`
- Create: `_audit/findings/04_net_comms.md`
- Create: `_audit/findings/05_system_ui_c5node.md`
- Create: `_audit/findings/06_ir_gps_specials.md`

- [ ] **Step 1: Send a single message with 6 Agent tool calls (see prompts in Appendix B)**

All 6 use `subagent_type: feature-dev:code-explorer`. Each prompt includes:
- The bucket file pattern
- Full contents of all 5 Wave-1 maps (paste them in)
- Audit criteria from spec §4
- Output path + format constraint (≤500 lines)
- Memory constraints from spec §5

- [ ] **Step 2: Verify all 6 artifacts exist and are non-empty**

```bash
ls -la _audit/findings/ && wc -l _audit/findings/*.md
```

Expected: 6 files, each ≤ ~550 lines (500 cap + slack).

- [ ] **Step 3: Smoke-check each findings file has the 6 required sections**

```bash
for f in _audit/findings/*.md; do
  echo "=== $f ==="
  grep -c -E "^## (Current state|Defects|Cross-ref deltas|Optimisation|Refactor targets|Backlog items)" "$f"
done
```

Expected: each file shows `6`.

- [ ] **Step 4: Smoke-check each Defects section cites real file:line**

```bash
grep -hE "src/[a-z_]+\.cpp:[0-9]+|src/features/[a-z_]+\.cpp:[0-9]+" _audit/findings/*.md | head -20
```

Expected: at least 20 distinct file:line citations across the 6 files.

### Task 2.2: Wave-2 review gate

- [ ] **Step 1: For each findings file, present user with: defect count, top 3 defects (severity + file:line), cross-ref delta tally (we_lack / we_have_better / parity / divergent)**

- [ ] **Step 2: User signals "go for wave 3" or requests auditor re-runs**

If a bucket is thin or wrong-scoped: re-dispatch with adjusted prompt.

### Task 2.3: Commit Wave-2 artifacts

- [ ] **Step 1: Stage and commit findings**

```bash
git add _audit/findings/
git commit -m "audit: wave 2 — per-module findings (WiFi/BLE/RF/Net/System/IR+specials)"
```

---

## Wave 3 — Synthesis

One sequential synthesis agent. Reads every `_audit/*` artifact, writes the 4 deliverables.

### Task 3.1: Dispatch synthesis agent

**Files (produced):**
- Create: `docs/audit/AUDIT.md`
- Create: `docs/audit/CROSS_REF.md`
- Create: `docs/audit/REFACTOR_PLAN.md`
- Create: `docs/audit/BACKLOG.md`

- [ ] **Step 1: Dispatch one synthesis agent (see prompt in Appendix C)**

`subagent_type: general-purpose`. Prompt includes:
- Full contents of all 5 maps and all 6 findings files
- Acceptance criteria from spec §6
- Deliverable templates from spec §2
- Memory constraints from spec §5

- [ ] **Step 2: Verify all 4 deliverables exist**

```bash
ls -la docs/audit/
```

Expected: 4 markdown files plus `.gitkeep`.

- [ ] **Step 3: Verify combined length ≤ 3000 lines (spec §6 acceptance criteria)**

```bash
wc -l docs/audit/AUDIT.md docs/audit/CROSS_REF.md docs/audit/REFACTOR_PLAN.md docs/audit/BACKLOG.md
```

Expected: total `≤ 3000`. If over, dispatch synthesis agent again with explicit re-scope instruction (NOT truncation — re-scope).

### Task 3.2: Cross-check defects against current code

- [ ] **Step 1: Extract every `file:line` citation in `AUDIT.md` and verify the file still exists**

```bash
grep -oE "src/[a-zA-Z0-9_/]+\.(cpp|h):[0-9]+" docs/audit/AUDIT.md | sort -u > /tmp/poseidon_audit_citations.txt
while IFS=: read -r path line; do
  if [ ! -f "$path" ]; then echo "MISSING FILE: $path"; fi
done < /tmp/poseidon_audit_citations.txt
```

Expected: zero `MISSING FILE` lines. Any miss → dispatch synthesis fixer.

- [ ] **Step 2: Verify every claim in `AUDIT.md` defect list maps to a backlog ticket**

```bash
grep -cE "^- \[" docs/audit/AUDIT.md
grep -cE "^### POS-AUDIT-" docs/audit/BACKLOG.md
```

Expected: backlog ticket count ≥ AUDIT defect count.

- [ ] **Step 3: Verify `REFACTOR_PLAN.md` has Phase 0–4 each with `Exit criteria:` line**

```bash
grep -cE "^## Phase [0-4]" docs/audit/REFACTOR_PLAN.md
grep -cE "^\*\*Exit criteria" docs/audit/REFACTOR_PLAN.md
```

Expected: both `5`.

### Task 3.3: Final commit

- [ ] **Step 1: Stage and commit deliverables**

```bash
git add docs/audit/
git commit -m "audit: wave 3 — synthesised AUDIT/CROSS_REF/REFACTOR_PLAN/BACKLOG"
```

- [ ] **Step 2: Show user the branch state**

```bash
git log --oneline master..HEAD
git diff --stat master..HEAD
```

Expected: 4 commits visible (scaffolding, wave 1, wave 2, wave 3). User decides next step (merge to master, open PR, continue on branch). Per POSEIDON release gate: **no push without explicit user approval.**

---

## Appendix A — Wave 1 agent prompts

### A1.1 — `poseidon-mapper` (subagent_type: `feature-dev:code-explorer`)

```
You are mapping the POSEIDON v0.6.1 firmware codebase. Output a single markdown
file at C:\Users\D\poseidon\_audit\maps\poseidon.md with the structure below.
Read-only task — do not modify any source.

SCOPE: C:\Users\D\poseidon\src\ (all .cpp/.h, recursive) and
       C:\Users\D\poseidon\src\features\ (all .cpp/.h) and
       C:\Users\D\poseidon\c5_node\ (all source files).

OUTPUT FORMAT (markdown):

  # POSEIDON v0.6.1 Inventory Map

  Generated 2026-06-01 from commit <run `git rev-parse --short HEAD` in repo>.

  ## Top-level groupings

  | Group | File count | LOC total | Notes |
  |-------|------------|-----------|-------|
  | System core (main/app/menu/ui/theme) | N | N | |
  | WiFi | N | N | |
  | BLE | N | N | |
  | Sub-GHz + RF radios (CC1101/nRF24/nRF52/LoRa) | N | N | |
  | Net attacks + comms (net_*/dhcp/mesh/satcom/c5) | N | N | |
  | IR / GPS / specials | N | N | |
  | c5_node (C5 ESP32 satellite) | N | N | |

  ## Per-file inventory

  ### src/

  - **main.cpp** — LOC, 1-line purpose. Gotcha tags: [TX-cache | WiFi-AP-IDF | BLE-coop | IR-active-low | GPS-off | HSPI-park | none].
  - **app.h** — ...
  - ...

  ### src/features/

  - **wifi_deauth.cpp** — LOC, purpose, gotcha tags.
  - ...

  ### c5_node/

  - ...

  ## Public API surface

  For headers that other modules depend on (anything #included by ≥3 files):
  list the exported symbols (functions, classes, typedefs).

  ## Gotcha tag distribution

  | Tag | Files | Notes |
  |-----|-------|-------|
  | TX-cache | ... | sprite caching during WiFi TX |
  | WiFi-AP-IDF | ... | raw IDF AP path (not Arduino WiFi.softAP) |
  | BLE-coop | ... | cooperative tick, not xTaskCreate |
  | IR-active-low | ... | GPIO 44, output_invert=1 |
  | GPS-off | ... | GPS tagging must default OFF |
  | HSPI-park | ... | SPI bus parking before nRF24/CC1101 |

GOTCHA TAG DEFINITIONS (apply when source pattern matches):
- TX-cache: file does pushImage/draw during a WiFi TX path → needs cache-to-SRAM
- WiFi-AP-IDF: file uses raw esp_wifi_set_config + softAP_t (good) OR uses
  WiFi.mode(WIFI_AP)/softAP (BAD — Bruce libs break this)
- BLE-coop: file creates NimBLE work inside a cooperative tick (good) OR uses
  xTaskCreate around NimBLE (BAD — silent rc=-1 fail)
- IR-active-low: file drives GPIO 44 for IR; needs output_invert=1, idle HIGH
- GPS-off: file writes capture metadata; gps tagging must be feature-gated OFF
- HSPI-park: file uses HSPI (SD/nRF24/CC1101) and must park other peripherals

LIMITS:
- File is unbounded but stay scannable — terse 1-line purpose per file.
- Do not paste source. Cite by path only.
```

### A1.2 — `bruce-mapper` (subagent_type: `general-purpose`)

```
You are mapping the Bruce firmware (by bmorcelli) for cross-reference against
POSEIDON. Output a single markdown file at
C:\Users\D\poseidon\_audit\maps\bruce.md.

SETUP:
  cd C:\Users\D\poseidon\_audit\refs
  git clone --depth=1 https://github.com/bmorcelli/Bruce.git bruce

POSEIDON FEATURE TAXONOMY (what to find in Bruce):
  WiFi: scan, deauth, beacon spam, probe, evil twin/portal, wardrive,
        PMKID, sanity override / raw frame TX, sniff/clients
  BLE: scan, spam, flood, sour apple, finder, findmy, karma, whisperpair,
       clone, GATT, HID, BlueDucky, MITM relay
  Sub-GHz: scan, replay, broadcast, jammer, jam-detect, bruteforce, spectrum,
           record
  CC1101 hardware: init, SPI setup, RMT TX/RX
  nRF24 hardware: init, mousejack, sniff
  IR: tvbgone, clone, remote
  Mesh / LoRa: mesh, position, status, nodes, page, chat, LoRa spectrum
  Net attacks: dhcp, lanrecon, responder, ssdp, wpad, cctv, evil portal HTTP
  Surveillance / drone: drone remoteid, surveillance hunter
  System: main, menu, UI/theme, input, screensaver, hat manager
  Special: badusb, satellite tracker, satcom

OUTPUT FORMAT (≤200 lines total):

  # Bruce Reference Map

  Commit: <hash>
  Cloned: 2026-06-01

  ## Feature index

  | Feature | File:line | Notes |
  |---------|-----------|-------|
  | WiFi scan | src/wifi/wifi_scan.cpp:12 | Uses esp_wifi_scan_start, no PSRAM |
  | WiFi deauth | src/wifi/deauth.cpp:45 | Raw frame TX path |
  | ... | ... | ... |

  ## Hardware bring-up patterns

  - CC1101 SPI setup: file:line — uses HSPI? FSPI? setSPIinstance?
  - nRF24 init: file:line — bus, pin parking pattern
  - WiFi raw frame TX: file:line — uses esp_wifi_80211_tx? rc handling?

  ## Build / lib pinning notes

  - platformio.ini key flags
  - Lib commit pins
  - -zmuldefs usage
  - Partition layout

LIMITS:
- ≤200 lines. Use the table form, no prose tours.
- Where Bruce splits a feature across many files, cite the entry point only.
- Note anything Bruce does that POSEIDON does NOT appear to do (high-level only;
  Wave 2 will do the per-module delta).
```

### A1.3 — `ghost-mapper` (subagent_type: `general-purpose`)

Same template as A1.2 with:
- Repo: `https://github.com/Spooks4576/Ghost_ESP.git ghost`
- Output: `_audit/maps/ghost.md`
- Header title: "Ghost ESP Reference Map"

### A1.4 — `porkchop-mapper` (subagent_type: `general-purpose`)

Same template as A1.2 with:
- Repo: `https://github.com/0ct0sec/M5PORKCHOP.git porkchop`
- Output: `_audit/maps/porkchop.md`
- Header title: "PORKCHOP Reference Map"
- Extra emphasis: M5PORKCHOP runs on Cardputer/CardputerAdv — note any
  freeze-resilience patterns (POSEIDON has an open Triton freeze regression).

### A1.5 — `evilm5-mapper` (subagent_type: `general-purpose`)

Same template as A1.2 with:
- Repo: `https://github.com/7h30th3r0n3/Evil-M5Project.git evilm5`
- Output: `_audit/maps/evilm5.md`
- Header title: "Evil-M5Project Reference Map"
- Extra emphasis: SaltyJack module in POSEIDON is an explicit port; flag every
  LAN/DHCP/Responder/NTLM/WPAD feature in Evil-M5 with file:line.

---

## Appendix B — Wave 2 auditor prompts

All six use this template. Substitute `<BUCKET>`, `<FILE_PATTERN>`, and `<OUTPUT_PATH>` per row in the table.

| Bucket # | `<BUCKET>` | `<FILE_PATTERN>` | `<OUTPUT_PATH>` |
|----------|-----------|-----------------|-----------------|
| 1 | WiFi | `src/features/wifi_*.cpp`, `src/features/wifi_*.h`, `src/features/evil_twin*`, `src/features/wifi_sanity_override.cpp`, `src/features/wifi_wardrive*`, `src/wifi_types.h`, `src/wifi_wardrive.h` | `_audit/findings/01_wifi.md` |
| 2 | BLE | `src/features/ble_*.cpp`, `src/features/ble_*.h`, `src/ble_db.cpp`, `src/ble_db.h`, `src/ble_types.h` | `_audit/findings/02_ble.md` |
| 3 | Sub-GHz + RF radios | `src/features/subghz_*`, `src/features/radio_lora.cpp`, `src/features/rf_finder.cpp`, `src/features/nrf24_suite.cpp`, `src/features/nrf52_*.cpp`, `src/cc1101_*`, `src/radio.cpp`, `src/radio.h`, `src/lora_hw.*`, `src/nrf24_hw.*`, `src/nrf24_types.h`, `src/nrf52_hw.*`, `src/nrf52_led.*`, `src/subghz_decode.*`, `src/subghz_types.h`, `src/ui_subghz.*` | `_audit/findings/03_subghz_rf.md` |
| 4 | Net attacks + comms | `src/features/net_*.cpp`, `src/features/feat_satcom.cpp`, `src/features/mesh_*.cpp`, `src/features/c5_scan.cpp`, `src/dhcp_cache.*`, `src/satcom.*`, `src/satcom_tle_baked.h`, `src/mesh.*`, `src/mesh/*`, `src/c5_cmd.*`, `src/net_helpers.*` | `_audit/findings/04_net_comms.md` |
| 5 | System / UI / c5_node | `src/main.cpp`, `src/app.h`, `src/menu*`, `src/ui.*`, `src/ui_ambient.*`, `src/theme.*`, `src/input.*`, `src/sfx.*`, `src/splash.cpp`, `src/screensaver.*`, `src/hat_manager.*`, `src/sd_helper.*`, `src/serial_test.*`, `src/features/screensaver_picker.cpp`, `src/features/sfx_settings.cpp`, `src/features/theme_picker.cpp`, `src/features/tools.cpp`, `src/features/system_tools.cpp`, `src/features/stubs.cpp`, `c5_node/**/*` | `_audit/findings/05_system_ui_c5node.md` |
| 6 | IR + GPS + specials | `src/features/ir_*`, `src/features/drone_remoteid.cpp`, `src/features/surveillance_hunter.cpp`, `src/features/defensive_monitor.cpp`, `src/features/badusb.cpp`, `src/features/mimir.cpp`, `src/features/mimir.h`, `src/features/trident.cpp`, `src/features/trident.h`, `src/features/triton.cpp`, `src/features/ap_signal_test.*`, `src/features/saltyjack/*`, `src/gps.*`, `src/argus.*`, `src/argus_data.h` | `_audit/findings/06_ir_gps_specials.md` |

### B-template — module bucket auditor (subagent_type: `feature-dev:code-explorer`)

```
You are auditing the POSEIDON v0.6.1 firmware module bucket: <BUCKET>.

REPO ROOT: C:\Users\D\poseidon

YOUR SCOPE — only these files:
<FILE_PATTERN>

INPUT CONTEXT — read these maps in full before starting:
- _audit/maps/poseidon.md   (POSEIDON inventory)
- _audit/maps/bruce.md      (Bruce reference index)
- _audit/maps/ghost.md      (Ghost ESP reference index)
- _audit/maps/porkchop.md   (PORKCHOP reference index)
- _audit/maps/evilm5.md     (Evil-M5 reference index)

OUTPUT: Write ONE markdown file at <OUTPUT_PATH>. ≤500 lines.

REQUIRED STRUCTURE (use these exact section headings):

  # <BUCKET> Module Audit

  ## Current state
  Brief tour — what the bucket does today, in <BUCKET> terms.
  Cite file:line for entry points.

  ## Defects
  Numbered list. For each: severity (CRIT/HIGH/MED/LOW), file:line,
  description, why-it-matters, suggested fix (1-line).
  Categories to scan for (in this order):

  1. Memory — heap budget assumptions, PSRAM-disabled safety (this unit's
     PSRAM is BROKEN — no code path may rely on it), allocator hotspots,
     leak paths, stack overflow risk.
  2. Hardware — SPI bus ownership (FSPI=display via M5GFX, HSPI=SD/nRF24/
     CC1101); pin conflicts vs platformio.ini header (frees GPIO
     3/4/5/6/7/13/15 for hats); IR active-low on GPIO 44 with output_invert
     and idle HIGH; ESP-NOW channel locking.
  3. Concurrency — BLE work must use cooperative tick (NimBLE init eats
     heap; xTaskCreate after NimBLE init fails silent rc=-1); NimBLE init
     order; ISR safety.
  4. Performance — TX layout sensitivity: pushImage during WiFi TX causes
     cache stall sprite scramble (cache to SRAM); refresh budget; deauth
     frame rate.
  5. OPSEC — GPS-tag default MUST be OFF unless user flips a switch;
     capture file handling; BLE/WiFi MAC randomisation; SD write paths.
  6. Build — Bruce libs pinned at 20260123 (NOT 20260407 which fails rc=258
     on-device); -zmuldefs override for wifi_sanity_override; partition
     layout vs 8MB flash; SmartRC pinned to bmorcelli commit
     7bba5217dc0632f718a3278dc27e401016741a95.
  7. Code quality — files >800 LOC flagged; duplication; dead code/stubs;
     gate consistency (hat-present checks, hardware-init guards).

  ## Cross-ref deltas
  For EACH reference firmware (Bruce, Ghost, Porkchop, EvilM5):
  - Per feature in your bucket, classify our position:
    - we_lack — they have it, we don't, brief note + their file:line
    - we_have_better — we do something they don't / better, why
    - parity — same capability, similar approach
    - divergent_approach — same capability, different approach, tradeoff
  Use a table per ref firmware.

  ## Optimisation opportunities
  Static-analysis perf wins distinct from defects. Each: file:line,
  observation, suggested change, expected gain (qualitative).

  ## Refactor targets
  Structural changes (file splits, API boundary cleanup, dead code removal,
  consolidation). Each: target, scope (files), rationale, risk.

  ## Backlog items
  Atomic tickets, one per defect/optimisation/refactor target. Format:

      - **<short-id>** [SEV] file:line — title. Fix: 1-line recipe. Effort: S/M/L.

  Atomic = does not bundle multiple unrelated changes. One fix at a time
  (POSEIDON is hardware-in-loop; bundling makes regression bisection
  impossible).

CONSTRAINTS:
- Read-only on src/. Do not modify anything outside <OUTPUT_PATH>.
- Every defect MUST cite file:line. No claims without coordinates.
- Cap output at 500 lines. If you exceed, drop the weakest LOW-severity
  defects first.
- Do not include source code blocks unless illustrating an exact fix
  recipe in ≤6 lines.
- Tone: terse, technical, no hedging.

When done, write the file and stop.
```

---

## Appendix C — Wave 3 synthesis prompt

### C-template — synthesis agent (subagent_type: `general-purpose`)

```
You are the synthesis agent for the POSEIDON v0.6.1 audit. Your inputs are
the 5 maps in _audit/maps/ and the 6 findings in _audit/findings/. You read
ALL of them, dedupe, prioritise, format. You do NOT do new code analysis.

REPO ROOT: C:\Users\D\poseidon

INPUT FILES (read in full):
  _audit/maps/poseidon.md
  _audit/maps/bruce.md
  _audit/maps/ghost.md
  _audit/maps/porkchop.md
  _audit/maps/evilm5.md
  _audit/findings/01_wifi.md
  _audit/findings/02_ble.md
  _audit/findings/03_subghz_rf.md
  _audit/findings/04_net_comms.md
  _audit/findings/05_system_ui_c5node.md
  _audit/findings/06_ir_gps_specials.md

DELIVERABLES — write all 4 to docs/audit/:

================================================================
1. docs/audit/AUDIT.md
================================================================

  # POSEIDON v0.6.1 — Audit Report

  ## Executive summary
  3–5 paragraphs. Headline state of the firmware. Key risks. Biggest
  surprises. Top 3 things to fix before any other work.

  ## Per-module scorecard

  | Module | Memory | Hardware | Perf | Quality | Overall |
  |--------|:------:|:--------:|:----:|:-------:|:-------:|
  | WiFi   | B+ | A- | B | B | B+ |
  | ...    | ... | ... | ... | ... | ... |

  Grading rubric (A=excellent, B=solid with minor issues, C=works but has
  notable defects, D=significant defects, F=broken). Justify each grade
  in 1 sentence under the table.

  ## Top 20 defects

  Ranked by severity then blast radius. Each entry:

  ### D01 — [CRIT] file:line — title

  - **Module:** <bucket>
  - **What:** 1-sentence problem statement
  - **Why it matters:** impact
  - **Fix:** 1-line recipe
  - **Backlog:** POS-AUDIT-NNN (link to BACKLOG.md ticket)

  ## Strengths
  3–5 bullet points — what POSEIDON does better than the 4 reference
  firmwares. This is morale + identity, keep it honest.

  ## Known blind spots
  Restate the §7 limits from the spec so future readers don't take
  AUDIT.md as exhaustive of runtime/UX bugs.

================================================================
2. docs/audit/CROSS_REF.md
================================================================

  # POSEIDON vs Bruce / Ghost ESP / PORKCHOP / Evil-M5 — Feature Matrix

  ## Matrix

  Rows = features (use POSEIDON's taxonomy from poseidon.md).
  Cols = POSEIDON | Bruce | Ghost | Porkchop | EvilM5.
  Cells = file:line + 1-symbol verdict:
    ✓ = implemented/parity
    ✓+ = implemented and best-in-class here
    ~ = partial/divergent approach
    ✗ = absent
    n/a = doesn't apply to this firmware's mission

  Group rows by module (WiFi / BLE / Sub-GHz / RF / Net / IR / GPS /
  System / Special).

  ## Gaps we should close
  Features marked ✗ in POSEIDON but ✓+ in any ref. Sorted by user-value.
  Each: feature, leader (which ref + their file:line), suggested entry
  point in POSEIDON.

  ## Leads we should keep
  Features marked ✓+ in POSEIDON. Each: feature, why we're ahead, what
  to protect during refactor.

  ## Divergent approaches worth re-examining
  Features marked ~ where the divergence has a real tradeoff. Each:
  feature, our approach, their approach, tradeoff, recommendation.

================================================================
3. docs/audit/REFACTOR_PLAN.md
================================================================

  # POSEIDON v0.6.1 → v0.7 Refactor Plan

  Five phases. Each phase has: goal, scope, dependencies, sequence,
  **Exit criteria:** line, and risk note.

  ## Phase 0 — Hygiene
  Build/lib pinning verification, partition cleanup, dead code/stub
  removal, .gitignore audit, doc drift. No behaviour changes.
  Dependencies: none.
  **Exit criteria:** ...

  ## Phase 1 — Hardware safety
  SPI bus ownership corrections, pin parking gates, IR polarity audit,
  ESP-NOW channel locks. Atomic per-defect.
  Dependencies: Phase 0.
  **Exit criteria:** ...

  ## Phase 2 — Module splits
  Files >800 LOC flagged for split; API boundary cleanup; gate
  consistency; consolidation of duplicated helpers.
  Dependencies: Phase 1 (don't refactor unstable surfaces).
  **Exit criteria:** ...

  ## Phase 3 — Feature parity
  Close the ✗ gaps from CROSS_REF.md that align with POSEIDON's mission.
  Dependencies: Phase 2.
  **Exit criteria:** ...

  ## Phase 4 — New capabilities
  Things no reference firmware has but POSEIDON should — driven by
  user-facing roadmap, not the audit. Stub the section if no items.
  Dependencies: Phase 3.
  **Exit criteria:** ...

  ## Sequencing notes
  - One-fix-at-a-time on hardware-in-loop changes.
  - POSEIDON release gate — no push/tag without explicit user approval
    per version cut.
  - Bruce lib bump (20260123 → newer) is a Phase 0 evaluation, not an
    automatic change — requires on-device regression sweep.

================================================================
4. docs/audit/BACKLOG.md
================================================================

  # POSEIDON Audit Backlog

  Source: 2026-06-01 v0.6.1 full audit.

  Ticket format:

  ### POS-AUDIT-001 — [CRIT][WiFi] file:line — title

  - **Severity:** CRIT/HIGH/MED/LOW
  - **Module:** WiFi/BLE/SubGHz/Net/System/IR
  - **Phase:** 0/1/2/3/4
  - **File:** src/features/foo.cpp:123
  - **Problem:** 2–3 sentences
  - **Fix recipe:** code-level instruction, ≤8 lines
  - **Effort:** S (≤1h) / M (≤4h) / L (>4h)
  - **Depends on:** other tickets (IDs) or "none"
  - **Verification:** how to confirm the fix on hardware (or "static only")
  - **Release gate:** user-gated yes/no

  Number tickets sequentially POS-AUDIT-001..NNN.
  Sort by severity (CRIT first), then phase, then module.

  Append an index table at top:
  | ID | Sev | Phase | Module | Title |
  |----|-----|-------|--------|-------|

ACCEPTANCE CRITERIA (verify before stopping):
- All 4 files exist and are non-empty.
- Combined total ≤ 3000 lines. If over, re-scope (drop weakest LOW-sev
  tickets and lowest-impact divergent-approach rows), do NOT truncate.
- Every defect in AUDIT.md top-20 has a corresponding ticket in BACKLOG.md.
- Every CROSS_REF.md "gaps we should close" entry has a corresponding
  ticket in BACKLOG.md (Phase 3).
- Every REFACTOR_PLAN.md phase has an Exit criteria: line.
- No file:line citation in AUDIT.md or BACKLOG.md is missing from the
  current checkout (you can verify by spot-checking with the file in repo).
- No placeholders, TBDs, "see above", or unresolved cross-refs.

When all 4 files are written and verified, stop.
```

---

## Out-of-band notes for the executor

- **Branch discipline.** All commits stay on `audit/v0.6.1`. Do NOT push to `origin` or create a PR without explicit user approval — POSEIDON release gate applies to anything that touches the public repo.
- **If a Wave-1 mapper fails the clone** (network, repo moved, etc.), do NOT skip — surface to user and pause. Reference maps drive Wave-2 cross-ref; without them the audit is half-blind.
- **If a Wave-2 auditor returns thin** (<150 lines, missing sections), re-dispatch with the same prompt — agent variance accounts for occasional shallow returns.
- **If Wave 3 deliverables exceed 3000 lines combined**, re-dispatch synthesis with an explicit instruction to drop the weakest LOW-severity tickets and lowest-impact divergent-approach rows. Do NOT truncate manually.
