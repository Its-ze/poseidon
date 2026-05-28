#!/usr/bin/env python3
"""
convert_argus.py — slice the Argus mood sprite sheet (6x2 grid) into
twelve 64x64 RGB565 sprites and emit a C header for the firmware.

Source: assets/argus/argus_sheet.png  (Gemini-generated, 1632x656,
        6 columns × 2 rows, 272x328 per cell, no borders/labels)
Output: src/argus_data.h

Each output sprite is 64x64 pixels = 4096 RGB565 words (8192 bytes).
Total payload across 12 moods: ~96 KB in flash (.rodata).

Render via M5Cardputer.Display.pushImage(x, y, 64, 64, ARGUS_<MOOD>).
"""
import sys
from pathlib import Path
from PIL import Image

ROOT = Path(__file__).resolve().parent.parent
SRC  = ROOT / "assets" / "argus" / "argus_sheet.png"
OUT  = ROOT / "src" / "argus_data.h"

# Target sprite size — 96x96. 50% bigger than the prior 64x64. Per-push
# cost is 18 KB but the cache in argus.cpp only pushes on mood/sway state
# change, so steady-state push rate is well under 1 Hz. Total flash
# footprint across 12 moods: ~221 KB.
SPR_W = 96
SPR_H = 96

# Sheet layout. 12 cells, raster order (row-major, left-to-right top-to-bottom).
# First 10 names match the prior enum so existing argus.cpp switch cases keep
# pointing at the right faces. CURIOUS + STERN are placeholders for the two
# new cells — rename in argus_data.h consumers if a better label fits.
MOODS = [
    "WATCHING",     # row 0, col 0
    "INTERESTED",   # row 0, col 1
    "PLEASED",      # row 0, col 2
    "ANNOYED",      # row 0, col 3
    "RESIGNED",     # row 0, col 4
    "CALCULATING",  # row 0, col 5
    "CYNICAL",      # row 1, col 0
    "OLD_FURY",     # row 1, col 1
    "SLEEPING",     # row 1, col 2
    "REFLECTIVE",   # row 1, col 3
    "CURIOUS",      # row 1, col 4 — NEW
    "STERN",        # row 1, col 5 — NEW
]

GRID_COLS = 6
GRID_ROWS = 2
BORDER_CROP_PX = 4           # tight inset to avoid edge bleed
LABEL_STRIP_BOTTOM_PX = 0    # new sheet has no label text under cells


def rgb_to_565(r: int, g: int, b: int) -> int:
    # The ST7789 SPI protocol clocks pixel bytes high-byte-first (BE).
    # ESP32 stores uint16_t in LE, so we pre-swap each word so that when
    # SPI transmits the in-memory bytes verbatim, the panel sees the
    # right RGB565 word. Without this, ivory skin (0xFEE6) reaches the
    # panel as 0xE6FE = lavender, and cyan beard (0x06FC) reaches as
    # 0xFC06 = yellow — which is exactly the user's "skin purplish,
    # beard yellow" complaint. (The earlier R/B swap was the wrong axis
    # — flipped both colors to "skin yellow, beard purple" instead.)
    word = ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3)
    return ((word & 0xFF) << 8) | ((word >> 8) & 0xFF)


def slice_cell(img: Image.Image, col: int, row: int) -> Image.Image:
    """Crop one cell out of the grid, trim the cyan border + label band."""
    cw = img.width  // GRID_COLS
    ch = img.height // GRID_ROWS
    x0 = col * cw + BORDER_CROP_PX
    y0 = row * ch + BORDER_CROP_PX
    x1 = (col + 1) * cw - BORDER_CROP_PX
    y1 = (row + 1) * ch - BORDER_CROP_PX - LABEL_STRIP_BOTTOM_PX
    # If the resulting box isn't square, center-crop on the larger axis
    # so the face doesn't end up squashed by the downsample.
    bw, bh = x1 - x0, y1 - y0
    if bw > bh:
        extra = (bw - bh) // 2
        x0 += extra
        x1 -= (bw - bh) - extra
    elif bh > bw:
        extra = (bh - bw) // 2
        y0 += extra
        y1 -= (bh - bw) - extra
    return img.crop((x0, y0, x1, y1))


def cell_to_rgb565(cell: Image.Image) -> list[int]:
    """Downsample to SPR_W x SPR_H and convert each pixel to RGB565."""
    cell = cell.convert("RGB")
    # LANCZOS for downsampling (NEAREST mangled colors at 6× downsample —
    # picks single source pixels and misses the eyes/highlights). LANCZOS
    # blends correctly while still preserving pixel-art edges at small
    # output sizes.
    cell = cell.resize((SPR_W, SPR_H), Image.LANCZOS)
    px = cell.load()
    out: list[int] = []
    for y in range(SPR_H):
        for x in range(SPR_W):
            r, g, b = px[x, y]
            out.append(rgb_to_565(r, g, b))
    return out


def emit_header(sprites: list[tuple[str, list[int]]]) -> str:
    lines: list[str] = []
    lines.append("/*")
    lines.append(" * argus_data.h — AUTO-GENERATED from assets/argus/argus_sheet.png.")
    lines.append(" * Regenerate: python scripts/convert_argus.py")
    lines.append(" *")
    lines.append(f" * {len(sprites)} mood sprites, {SPR_W}x{SPR_H} RGB565, native endian.")
    lines.append(" * Render via M5Cardputer.Display.pushImage(x, y, ARGUS_W, ARGUS_H, ARGUS_<MOOD>).")
    lines.append(" */")
    lines.append("#pragma once")
    lines.append("")
    lines.append(f"#define ARGUS_W  {SPR_W}")
    lines.append(f"#define ARGUS_H  {SPR_H}")
    lines.append(f"#define ARGUS_PIXELS  ({SPR_W} * {SPR_H})")
    lines.append("")
    lines.append("/* Mood enum — values are the row-major index in the source sheet. */")
    lines.append("enum argus_mood_t : int {")
    for i, (name, _) in enumerate(sprites):
        lines.append(f"    ARGUS_{name} = {i},")
    lines.append(f"    ARGUS_MOOD_COUNT = {len(sprites)},")
    lines.append("};")
    lines.append("")
    for name, words in sprites:
        lines.append(f"static const uint16_t ARGUS_SPR_{name}[ARGUS_PIXELS] = {{")
        per_line = 12
        for i in range(0, len(words), per_line):
            chunk = words[i:i + per_line]
            row = "    " + ", ".join(f"0x{w:04X}" for w in chunk) + ","
            lines.append(row)
        lines.append("};")
        lines.append("")
    lines.append("/* Table of sprite pointers for runtime indexing. */")
    lines.append("static const uint16_t *const ARGUS_SPRITES[ARGUS_MOOD_COUNT] = {")
    for name, _ in sprites:
        lines.append(f"    ARGUS_SPR_{name},")
    lines.append("};")
    return "\n".join(lines)


def main() -> int:
    if not SRC.exists():
        print(f"ERROR: source not found: {SRC}", file=sys.stderr)
        return 1
    img = Image.open(SRC).convert("RGB")
    print(f"source: {SRC} ({img.size[0]}x{img.size[1]})", file=sys.stderr)

    sprites: list[tuple[str, list[int]]] = []
    for i, name in enumerate(MOODS):
        col = i % GRID_COLS
        row = i // GRID_COLS
        cell = slice_cell(img, col, row)
        words = cell_to_rgb565(cell)
        sprites.append((name, words))
        print(f"  {name:12s} cell=({col},{row})  cell_size={cell.size}  words={len(words)}",
              file=sys.stderr)

    OUT.parent.mkdir(parents=True, exist_ok=True)
    OUT.write_text(emit_header(sprites))
    bytes_total = len(sprites) * SPR_W * SPR_H * 2
    print(f"wrote: {OUT}  ({bytes_total // 1024} KB total)", file=sys.stderr)
    return 0


if __name__ == "__main__":
    sys.exit(main())
