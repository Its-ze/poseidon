#!/usr/bin/env python3
"""Build the ARGUS RAG knowledge base from the project's own sources.

ARGUS is the POSEIDON site chatbot. Rather than fine-tune (expensive, goes
stale on every firmware change), we retrieve over a knowledge base extracted
straight from the repo, so it always reflects the current project.

Sources, richest first:
  docs/codex.html  -> the BANDS array: every radio + every tool (name/what/how)
  README.md        -> project overview, chunked by '## ' headings
  c5_node/README.md-> TRIDENT C5 protocol + build
  docs/*.html      -> hardware / install / legal / roadmap (tag-stripped)

Output: docs/assets/argus-kb.json  — [{id, source, title, text}, ...]
Run:    python3 tools/build_argus_kb.py
"""
import json, re, os, sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
def rd(p):
    fp = os.path.join(ROOT, p)
    return open(fp, encoding="utf-8").read() if os.path.exists(fp) else ""

STR = r'"((?:\\.|[^"\\])*)"'        # a JS double-quoted string with escapes
def unesc(s):
    return s.replace('\\"', '"').replace('\\\\', '\\').replace('\\n', ' ').strip()
def detag(s):
    s = re.sub(r'<script\b[^>]*>.*?</script>', ' ', s, flags=re.S|re.I)
    s = re.sub(r'<style\b[^>]*>.*?</style>', ' ', s, flags=re.S|re.I)
    s = re.sub(r'<[^>]+>', ' ', s)
    s = (s.replace('&amp;', '&').replace('&lt;', '<').replace('&gt;', '>')
           .replace('&nbsp;', ' ').replace('&mdash;', '-').replace('&copy;', '(c)'))
    return re.sub(r'\s+', ' ', s).strip()

chunks = []
def add(source, title, text):
    text = text.strip()
    if len(text) > 25:
        chunks.append({"id": f"k{len(chunks):03d}", "source": source, "title": title, "text": text})

# ---- 1. codex BANDS: the per-tool gold ----
codex = rd("docs/codex.html")
m = re.search(r'const BANDS\s*=\s*\[(.*?)\];', codex, re.S)
if not m:
    print("ERROR: could not locate BANDS array in docs/codex.html"); sys.exit(1)
bands_src = m.group(1)
# split into band blocks at each top-level "{ id:"
blocks = re.split(r'\{\s*id:', bands_src)
for blk in blocks[1:]:
    def field(name):
        mm = re.search(name + r'\s*:\s*' + STR, blk)
        return unesc(mm.group(1)) if mm else ""
    bid   = unesc(re.match(r'\s*' + STR, blk).group(1)) if re.match(r'\s*' + STR, blk) else "?"
    name  = field("name"); sub = field("sub"); chip = field("chip")
    count = field("count"); intro = field("intro")
    add(f"codex/{bid}", f"{name} radio ({chip})",
        f"{name} ({chip}, {sub}) — {count}. {intro}")
    for tm in re.finditer(r'\{\s*n:\s*' + STR + r'\s*,\s*w:\s*' + STR + r'\s*,\s*h:\s*' + STR + r'\s*\}', blk):
        n, w, h = (unesc(tm.group(1)), unesc(tm.group(2)), detag(unesc(tm.group(3))))
        add(f"codex/{bid}", f"{name} > {n}",
            f"{n} (a {name} / {chip} tool). What it does: {w} How it works: {h}")

# ---- 2. README.md, chunked by '## ' headings ----
for fname in ("README.md", "c5_node/README.md"):
    md = rd(fname)
    if not md: continue
    parts = re.split(r'\n(?=#{1,3}\s)', md)
    for p in parts:
        hm = re.match(r'#{1,3}\s*(.+)', p)
        title = hm.group(1).strip() if hm else fname
        body = re.sub(r'`{1,3}', '', re.sub(r'\s+', ' ', p)).strip()
        add(fname, title, body[:1400])

# ---- 3. supporting pages (tag-stripped, ~700-char chunks) ----
for page, label in [("docs/hardware.html","Hardware"), ("docs/install.html","Install / flashing"),
                    ("docs/legal.html","Legal / authorized use"), ("docs/roadmap.html","Roadmap")]:
    txt = detag(rd(page))
    for i in range(0, len(txt), 700):
        add(page, label, txt[i:i+760])

out = os.path.join(ROOT, "docs/assets/argus-kb.json")
json.dump(chunks, open(out, "w", encoding="utf-8"), ensure_ascii=False, indent=0)
print(f"wrote {len(chunks)} chunks -> docs/assets/argus-kb.json "
      f"({os.path.getsize(out)//1024} KB)")
# quick source histogram
from collections import Counter
for s, c in sorted(Counter(k['source'] for k in chunks).items()):
    print(f"  {c:>3}  {s}")
