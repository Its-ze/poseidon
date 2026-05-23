# POSEIDON Release Checklist

Operational cheat sheet — everywhere that has to be touched when
cutting a new POSEIDON version. Generated from the v0.5.0 → 0.6 push
audit (2026-05-22) where multiple stale strings + an untracked
`docs/flash/` dir + Jekyll silently failing bit us.

If you're cutting a release and reading this, work top-to-bottom.

---

## 0. Pre-flight

- [ ] Confirm the gating feedback rule: **do not push, tag, or release
      POSEIDON without explicit user go-ahead each time** (per
      `feedback_poseidon_github_gate.md`).
- [ ] `git status` clean except for files you intend to commit.
- [ ] All hardware-in-loop tests in the previous version still pass —
      regression sweep at minimum (boot, menu, one leaf per submenu).
- [ ] Pick the next version number (SemVer — bug-fix → patch,
      backward-compatible feature → minor, breaking change → major).

## 1. Source-code version constants

- [ ] `platformio.ini` → `-DPOSEIDON_VERSION=\"X.Y.Z\"` build flag.
      `src/version.h` reads it. Boot splash + serial harness use it.
- [ ] `c5_node/main/proto.h` → `POSEI_VERSION` if the wire protocol
      changed. Bump only when CMD/RESP IDs change — most C5 commits
      don't need this.
- [ ] If the C5 firmware version is independent of POSEIDON, also
      record it in `docs/flash/manifest-trident-c5.json` "version"
      field (currently `proto-vN-YYYY-MM-DD`).

## 2. Build fresh artifacts

- [ ] **POSEIDON (S3, Cardputer-Adv):**

  ```
  PYTHONIOENCODING=utf-8 PYTHONUTF8=1 \
    /c/Users/D/.platformio/penv/Scripts/pio.exe run -e cardputer
  ```

  Produces `.pio/build/cardputer/firmware.factory.bin`. The launcher
  variant (`-e cardputer-launcher`) is a separate build with a
  different partition table.

- [ ] **TRIDENT (C5):**

  ```
  cd c5_node
  ./_build.bat build
  ```

  Produces `c5_node/build/{bootloader,partition_table,poseidon_c5_node}.bin`.

- [ ] **TRIDENT factory.bin** (single combined image for the
      `install.html` flow) — concatenate the three C5 bins at their
      flash offsets:

  ```
  cd c5_node/build
  esptool.py --chip esp32c5 merge_bin -o trident-factory.bin \
    --flash_mode dio --flash_freq 80m --flash_size 4MB \
    0x2000  bootloader/bootloader.bin \
    0x8000  partition_table/partition-table.bin \
    0x10000 poseidon_c5_node.bin
  ```

  This is what gets attached to the GitHub release for the
  install.html → `manifest-trident.json` web-flash flow.

## 3. Refresh web flasher binaries (Pages-served)

- [ ] Copy fresh C5 bins into the docs flasher tree:

  ```
  cp c5_node/build/bootloader/bootloader.bin            docs/flash/bin/trident-c5/
  cp c5_node/build/partition_table/partition-table.bin  docs/flash/bin/trident-c5/
  cp c5_node/build/poseidon_c5_node.bin                 docs/flash/bin/trident-c5/
  ```

  Also mirror into `docs/bin/trident-c5/` (duplicate location kept
  for legacy references — delete this when nothing points at it).

- [ ] `.gitignore` already has `!docs/flash/bin/**/*.bin` +
      `!docs/bin/**/*.bin` negation rules so the bins ARE trackable.
      Verify with `git check-ignore -v docs/flash/bin/trident-c5/*.bin`.

- [ ] Bump `version` in `docs/flash/manifest-trident-c5.json`.

- [ ] Sanity-check the manifest is **pure ASCII** — UTF-8 em-dashes
      in past versions got cp1252-encoded by editors and broke
      browser parsing. The current file uses `-` not `—` for safety:

  ```
  python -c "raw=open('docs/flash/manifest-trident-c5.json','rb').read(); print('all ASCII:', all(b<0x80 for b in raw))"
  ```

## 4. Documentation updates

The complete touch list from the v0.5.0 → 0.6 audit:

### Repo docs

- [ ] **`CHANGELOG.md`** — promote `[Unreleased]` to `[X.Y.Z] - YYYY-MM-DD`,
      open a new empty `[Unreleased]` section above it.
- [ ] **`README.md`** — check these specifically:
  - Feature count in the hero blurb ("95+ features")
  - Feature count in the section header ("## Feature Matrix (95+)")
  - Per-section counts (WiFi 17, BLE 16, IR 4, etc.) — count menu
    entries in `src/menu.cpp` if a section grew
  - Theme count + theme list (currently 3: POSEIDON / MATRIX / E-INK)
  - Quick Start binary URL — points at `releases/latest/download/...`
    which auto-redirects, no version edit needed unless the asset
    name itself changes
- [ ] **`TESTERS.md`** — "Currently testing" line at the top,
      priority list ordering, anything that says "vX.Y.Z shipped".
- [ ] **`KnownDeauthBugFixSoon.md`** — refresh if any known bugs got
      fixed in this version, or new ones surfaced.
- [ ] **`c5_node/README.md`** — protocol version, build flow.

### Website pages (served from `docs/` via GitHub Pages)

- [ ] **`docs/index.html`** — four spots that go stale:
  - `<div id="hud">` HUD text (`LIVE · vX.Y.Z ...`)
  - `<div class="hero-badge">` (the lit banner above the hero title)
  - `<div class="hero-stats">` numbers (Features / RF + Wired / Themes
    / .sub Signals)
  - `<div class="hero-btns">` if a new CTA replaces an old one
- [ ] **`docs/install.html`** — the "once we tag vX.Y.Z" M5Burner
      sentence + the esp-web-tools script tag URL (currently
      `jsdelivr@10.2.1`, do NOT switch to unpkg, see §6).
- [ ] **`docs/arsenal.html`** — `v0.X HEADLINE` / `v0.X FIXED` tags
      on feature cards are historical labels; you can add new ones
      for the current version's headline features.
- [ ] **`docs/credits.html`** — `vX.Y headline credit` mentions if
      this version has a credit that headlines the release.
- [ ] **`docs/hardware.html`** — only touch if hardware list changes
      (new hat, new MCU variant, etc.).
- [ ] **`docs/roadmap.html`** — move items from "in flight" to
      "shipped"; add new "in flight" items for next milestone.
- [ ] **`docs/testers.html`** — mirror of `TESTERS.md` priorities;
      HUD string + meta description.
- [ ] **`docs/flash/index.html`** — only touch if you're adding a
      new target board card (e.g. POSEIDON itself once we get web
      flashing for the S3, or a new satellite).

### Manifests

- [ ] **`docs/flash/manifest-trident-c5.json`** — bump version, ASCII-clean.
- [ ] **`docs/manifest-trident.json`** — install.html's release-URL
      manifest. The path itself (`releases/latest/download/trident-factory.bin`)
      doesn't need editing because `latest` auto-resolves, but bump
      the `"version"` field for clarity.
- [ ] **`docs/m5burner.json`** — if we publish via M5Burner. The
      `install.html` text says "once we tag vX.Y.Z you'll see it
      listed" — verify M5Burner is actually consuming this json
      before relying on that line.

## 5. Commit + push order

```
# Stage doc updates separately from version bumps if you want a
# clean release commit history. Otherwise one commit is fine.
git add CHANGELOG.md README.md TESTERS.md docs/
git add platformio.ini c5_node/main/proto.h   # if version constants changed
git add docs/flash/bin/trident-c5/            # if rebuilt
git commit -m "release: vX.Y.Z"

# Tag and push BEFORE creating the release, so the release can
# reference the tag.
git tag vX.Y.Z
git push origin master --tags
```

## 6. GitHub release

```
gh release create vX.Y.Z \
  --title "POSEIDON vX.Y.Z — <release codename>" \
  --notes-file CHANGELOG-extract.md \
  .pio/build/cardputer/firmware.factory.bin#poseidon-factory.bin \
  .pio/build/cardputer-launcher/firmware.factory.bin#poseidon-launcher.bin \
  c5_node/build/trident-factory.bin#trident-factory.bin
```

`CHANGELOG-extract.md` is just the `[X.Y.Z]` section copy-pasted from
`CHANGELOG.md`. The `#friendly-name.bin` suffix renames the uploaded
asset so it matches what `install.html` + `manifest-trident.json`
expect (`releases/latest/download/poseidon-factory.bin` etc.).

## 7. Post-push verification

GitHub Pages auto-builds from `master:/docs` on every push. Build
takes ~30-60s. Poll the live page until it serves the new content:

```
until curl -s https://generaldussduss.github.io/poseidon/ | grep -q "vX.Y.Z"; do sleep 5; done
echo "live"
```

Check each endpoint independently:

```
curl -s -o /dev/null -w "%{http_code}\n" https://generaldussduss.github.io/poseidon/
curl -s -o /dev/null -w "%{http_code}\n" https://generaldussduss.github.io/poseidon/flash/
curl -s -o /dev/null -w "%{http_code}\n" https://generaldussduss.github.io/poseidon/flash/manifest-trident-c5.json
curl -s -o /dev/null -w "%{http_code} %{size_download}\n" \
  https://generaldussduss.github.io/poseidon/flash/bin/trident-c5/poseidon_c5_node.bin
```

If anything 404s, check the Pages build log:

```
gh run list -L 3 -w "pages-build-deployment"
gh run view <id> --log-failed
```

If the release-asset flow (`install.html`) is broken, check the
asset list:

```
gh release view vX.Y.Z --json assets --jq '.assets[].name'
```

## 8. Common gotchas (each cost a debug round)

- **`*.bin` is gitignored.** Anything you copy into `docs/` won't
  appear in `git status` without the negation rules. Verify with
  `git check-ignore -v <file>`.
- **`docs/.nojekyll` must exist.** Without it, GitHub Pages runs
  Jekyll, which chokes on any `{{` literal in any md file under docs/
  (Python f-strings under `docs/superpowers/plans/` killed the build
  for the entire site on the v0.5.0 → 0.6 push). Build silently
  reports `building` while the worker shows red.
- **Pages serves from `master:/docs`** not `main`. Don't rebase onto
  a `main` branch and assume Pages will pick it up — config it first.
- **Em-dash encoding hazard.** Windows editors save manifest JSON as
  cp1252 (em-dash = byte `0x97`), browsers parse as UTF-8 and see an
  invalid byte. Stick to ASCII `-` in JSON or verify the file is
  proper UTF-8 (`raw[i] for i where 0xC2 <= raw[i] <= 0xF4`).
- **esp-web-tools CDN.** `cdn.jsdelivr.net/npm/esp-web-tools@10.2.1`
  works. `unpkg.com/esp-web-tools@10` serves edge-cached 10.0-era
  bundles that lack ESP32-C5 support — users see false "Browser not
  supported" because the custom-element script never registers.
- **ESP32-C5 + Cardputer-Adv share USB VID/PID** (0x303A:0x1001).
  When both are plugged in, esp-web-tools will let you pick either
  port but flash will fail with "Failed to initialize" if you pick
  the wrong chip (S3 vs C5). Test by enumeration order or USB
  device path.
- **C5 doesn't always auto-reset into download mode.** Hold BOOT
  (GPIO 9), tap RESET, release BOOT. Some C5 dev boards lack the
  DTR/RTS auto-reset wiring.

## 9. Things that auto-update (don't touch)

- `releases/latest/download/<asset>.bin` URLs — GitHub auto-resolves
  `latest` to whatever tag was published most recently. No manifest
  edit needed unless the asset *name* changes.
- POSEIDON's boot splash version — read from
  `-DPOSEIDON_VERSION=...` in `platformio.ini` at compile time via
  `src/version.h`. Only the build flag needs editing.
- The c5_node build's flash offsets (`build/flash_args`) are
  regenerated by IDF every build; don't hand-edit.

## 10. Update memory after a release

Update `~/.claude/projects/.../memory/` so future-me knows where things landed:

- [ ] `project_poseidon.md` — bump current version, summarize the
      release headline.
- [ ] Any `feedback_poseidon_*` memory whose recipe changed (e.g.
      WiFi AP recipe, IR LED polarity) — verify still accurate.
- [ ] Memory pointer to this file (`RELEASE_CHECKLIST.md`) in
      `MEMORY.md` index — already in place.
