#!/usr/bin/env python3
"""
convert_icons.py — slice icons.jpg (4x4 grid, 14 icons + 2 empty cells)
into 14 individual 24x24 1-bit bitmap C arrays compatible with M5GFX
drawBitmap().

Each cell in the source is roughly 256x256 px; we crop, downsample to
24x24 with Lanczos, threshold the magenta strokes against the black
background, and pack MSB-first row-by-row (3 bytes per row, 72 bytes
per icon).

Output is a single header file at src/menu_icons_data.h that the
dispatcher in menu_icons.cpp pulls in.
"""
import sys
from pathlib import Path
from PIL import Image, ImageOps

SRC = Path(__file__).resolve().parent.parent / "assets" / "icons" / "icons.jpg"
OUT = Path(__file__).resolve().parent.parent / "src" / "menu_icons_data.h"

# Hotkey -> grid cell index (row-major), matched to the icon sheet layout.
LAYOUT = [
    ("wifi",      0),
    ("ble",       1),
    ("ir",        2),
    ("trident",   3),
    ("usb",       4),
    ("network",   5),
    ("skull",     6),
    ("radio",     7),
    ("tools",     8),
    ("mesh",      9),
    ("satellite", 10),
    ("eye",       11),
    ("laptop",    12),
    ("gear",      13),
]

ICON_W = 24
ICON_H = 24
THRESHOLD = 70   # tune if strokes come out too thin / too thick

def slice_cell(img: Image.Image, idx: int) -> Image.Image:
    cw = img.width // 4
    ch = img.height // 4
    col = idx % 4
    row = idx // 4
    return img.crop((col * cw, row * ch, (col + 1) * cw, (row + 1) * ch))

def to_bitmap(cell: Image.Image) -> bytes:
    # Tight crop around the visible glyph in the cell so the resize
    # doesn't waste resolution on padding. Use the alpha-equivalent
    # mask: anything not pure black is "ink".
    g = cell.convert("L")
    bbox = g.point(lambda v: 255 if v > 30 else 0).getbbox()
    if bbox:
        # Add a 6 px padding so the icon doesn't touch the cell edges
        # post-resize (purely visual breathing room).
        pad = 8
        x0, y0, x1, y1 = bbox
        x0 = max(0, x0 - pad); y0 = max(0, y0 - pad)
        x1 = min(cell.width, x1 + pad); y1 = min(cell.height, y1 + pad)
        # Make the bbox square so the icon doesn't get squished.
        bw = x1 - x0; bh = y1 - y0
        if bw > bh:
            extra = (bw - bh) // 2
            y0 = max(0, y0 - extra); y1 = min(cell.height, y1 + extra)
        elif bh > bw:
            extra = (bh - bw) // 2
            x0 = max(0, x0 - extra); x1 = min(cell.width, x1 + extra)
        g = g.crop((x0, y0, x1, y1))

    g = g.resize((ICON_W, ICON_H), Image.LANCZOS)

    # Threshold to 1-bit. Anything brighter than THRESHOLD becomes ink.
    out = bytearray()
    for y in range(ICON_H):
        byte = 0
        bit = 7
        for x in range(ICON_W):
            v = g.getpixel((x, y))
            if v > THRESHOLD:
                byte |= (1 << bit)
            bit -= 1
            if bit < 0:
                out.append(byte)
                byte = 0
                bit = 7
        # ICON_W=24 is a multiple of 8 so we always close cleanly.
    return bytes(out)

def emit_header(arrays: list[tuple[str, bytes]]) -> str:
    lines = []
    lines.append("/*")
    lines.append(" * menu_icons_data.h — AUTO-GENERATED from assets/icons.jpg.")
    lines.append(" * Regenerate: python scripts/convert_icons.py")
    lines.append(" *")
    lines.append(f" * 14 icons, {ICON_W}x{ICON_H} 1-bit, MSB-first row packing.")
    lines.append(" * Render via M5Cardputer.Display.drawBitmap(x, y, ICON_*, ICON_W, ICON_H, color).")
    lines.append(" */")
    lines.append("#pragma once")
    lines.append("")
    lines.append(f"#define MENU_ICON_W  {ICON_W}")
    lines.append(f"#define MENU_ICON_H  {ICON_H}")
    lines.append("")
    for name, data in arrays:
        lines.append(f"static const uint8_t MENU_ICON_{name.upper()}[{len(data)}] = {{")
        per_line = 12
        for i in range(0, len(data), per_line):
            chunk = data[i:i + per_line]
            row = "    " + ", ".join(f"0x{b:02X}" for b in chunk) + ","
            lines.append(row)
        lines.append("};")
        lines.append("")
    return "\n".join(lines)

def main():
    if not SRC.exists():
        print(f"ERROR: source not found: {SRC}", file=sys.stderr)
        sys.exit(1)
    img = Image.open(SRC).convert("RGB")
    print(f"source: {SRC} ({img.size[0]}x{img.size[1]})", file=sys.stderr)

    arrays: list[tuple[str, bytes]] = []
    for name, idx in LAYOUT:
        cell = slice_cell(img, idx)
        bits = to_bitmap(cell)
        arrays.append((name, bits))
        ink = sum(bin(b).count("1") for b in bits)
        print(f"  {name:10s} cell={idx:2d}  bytes={len(bits)}  ink_pixels={ink}", file=sys.stderr)

    OUT.parent.mkdir(parents=True, exist_ok=True)
    OUT.write_text(emit_header(arrays))
    print(f"wrote: {OUT}", file=sys.stderr)

if __name__ == "__main__":
    main()
