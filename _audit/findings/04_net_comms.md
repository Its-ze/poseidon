# Net Attacks + Comms Module Audit

## Current state

POSEIDON's "net attacks + comms" bucket is 26 files / ~9.6 KLOC and splits into three orthogonal stacks. Stack 1 is **LAN / Internet offensive features** all gated on a prior STA association (or, for AP-mode features, raw-IDF AP bringup): `net_attacks.cpp` (6 features — UART shell `feat_uart_shell` @net_attacks.cpp:89, reverse TCP `feat_tcp_tunnel` @:222, telnet honeypot `feat_honeypot` @:348, WiFi dead drop `feat_dead_drop` @:547, printer port-9100 detect `feat_printer` @:614, SSDP poisoner `feat_ssdp_poison` @:735), `net_dhcp.cpp` (`feat_dhcp_starve` @:124, `feat_dhcp_rogue_sta/ap` @:413/414 via `rogue_dhcp_loop` @:320, `feat_net_hijack` @:421), `net_wpad.cpp` (`feat_wpad_abuse` @:274, `feat_autodiscover` @:505), `net_responder.cpp` (`feat_net_responder` @:176 — LLMNR + NBT-NS + mDNS poisoner + SMB-445 NTLMv2 stub), `net_lanrecon.cpp` (`feat_net_lanrecon` @:366 — ARP+ports+banner+OUI+CSV), `net_cctv.cpp` (`feat_cctv_scan` @:542, three modes: LAN sweep, single IP, /poseidon/cctv-targets.txt), `net_tools.cpp` (`feat_net_portscan`/`feat_net_ping`/`feat_net_dns` @:39/94/126), `net_ssdp.cpp` (`feat_net_ssdp` @:74 — M-SEARCH burst + LOCATION XML parse). Shared helpers (`net_helpers.cpp`) centralize `net_tcp_open` @:6 and HTTP-GET parser `net_http_get` @:18.

Stack 2 is **C5 / Trident satellite control**: `c5_cmd.cpp` is the POSEIDON-side wire-protocol-v3 client (HELLO peering, command senders 10-29, RESP routing 40-49); `c5_cmd.h` is the wire definition mirrored from `c5_node/main/proto.h`. `c5_scan.cpp` runs the dashboards: `feat_c5_status` @:169, `feat_c5_scan_5g` @:233, `feat_c5_deauth_5g` @:368, `feat_c5_scan_zb` @:504, `feat_c5_pmkid_5g` @:639, `feat_c5_nuke_5g` @:779, and the shared `c5_deauth_dashboard` @:27. `dhcp_cache.cpp` is the shared (MAC→hostname) cache (24-entry, oldest-evict) populated by `dhcp_try_parse_802_11` @:55.

Stack 3 is **mesh + comms**. Two unrelated mesh stacks coexist on a single radio domain assumption: **ESP-NOW PigSync** (`mesh.cpp` + `mesh.h`, 180 LOC, HELLO every 5 s, GPS-included if fix, peer table @8) viewed by `feat_mesh` (`mesh_status.cpp`), and the **Meshtastic LoRa leaf** (`mesh/meshtastic_node.cpp` 591 LOC + `mesh/meshtastic_crypto.cpp` 60 LOC + `mesh/meshtastic_pb.cpp` 353 LOC) viewed by `mesh_chat.cpp` / `mesh_nodes.cpp` / `mesh_page.cpp` / `mesh_position.cpp`. Finally, `satcom.cpp` + `feat_satcom` (`feat_satcom.cpp:222`) does TLE fetch + SGP4 propagation against `satcom_tle_baked.h` (14 baked NORADs).

## Defects

### 1. Resource ownership / radio coordination

- **c5_cmd.cpp:260** unconditionally `esp_wifi_set_channel(1, ...)` on every `c5_begin()`. Mesh.cpp's PigSync expects channel-1 too, so they agree. But `c5_status` / `feat_c5_*` are called from menus that don't tear down WiFi features first — a user exiting `feat_wifi_scan` (channel-hopping STA scan ends on its last channel) into a C5 feature has their primary radio yanked to ch 1 without notifying any concurrent `feat_mesh` task (`mesh.cpp:111 mesh_task`) or wardrive worker. No abort/coordination signal. **Severity:** med — explained behavior in comments but still a silent invariant break.
- **mesh.cpp:139** `WiFi.getMode()` check is exactly the anti-pattern flagged in `c5_cmd.cpp:248-253` block (Arduino getMode() lies when raw-IDF is used). `mesh_begin` from a path that just ran a raw-IDF feature (Triton, portal, deauth) will hit `WiFi.mode(WIFI_STA)` which double-creates the netif and asserts in `esp_netif_create_default_wifi_sta`. C5 has the fix; mesh.cpp doesn't.
- **net_dhcp.cpp:557** (read: line 327) — `feat_dhcp_rogue_ap` uses `WiFi.softAP("FreeWiFi", ...)` (Arduino path), violating the [WiFi-AP-IDF] invariant. Same for **net_attacks.cpp:558** dead-drop and **net_wpad.cpp:279/512** WPAD/Autodiscover. Bruce-libs sanity override is hoping these still come up, but they're a known instability surface.
- **net_responder.cpp:201** `xTaskCreate(smb_task, ..., 4096, ...)` runs WiFiServer accept loop on a task without channel-lock cooperation with mesh — and although `s_smb`/`s_smb_alive` plus a 1-s join handshake is implemented, the design contradicts the "POSEIDON BLE/WiFi cooperative tick" feedback item by adding a hidden task for *every* responder run.

### 2. Wire protocol v3 drift (S3 ↔ C5)

`src/c5_cmd.h` ↔ `c5_node/main/proto.h` overall match. Verified opcodes 10-26 and 40-49 line up. Struct shapes line-by-line match (`c5_ap_t` / `posei_ap_t`, `c5_hs_t` / `posei_hs_t`, `c5_clients_req_t` / `posei_clients_req_t`, `c5_spectrum_t` / `posei_spectrum_t`, etc.). Two drifts to flag:

- **c5_cmd.h:31 vs proto.h:34** name drift: S3 calls type 17 `C5_TYPE_CMD_HS`, C5 calls it `POSEI_TYPE_CMD_HS_CAPTURE`. Same value; cosmetic only.
- **Missing `posei_scan_req_t` equivalent on S3 side.** `proto.h:98-100` defines `posei_scan_req_t { uint16_t duration_ms }`. S3 sends raw `uint16_t d` instead of a typed struct (`c5_cmd.cpp:391-394`). Wire-compatible but adds a manual encoder where the C struct should be shared.
- **c5_cmd.cpp:441-516** explicitly admits "C5-side dispatcher does NOT yet handle these CMD types — frames silently dropped" for the new v3 commands. That's coordinated work-in-progress, not a defect, but worth recording: any UI surfacing for `c5_cmd_clients_*`, `c5_cmd_beacon_spam`, `c5_cmd_probe_sniff`, `c5_cmd_deauth_detect`, `c5_cmd_karma`, `c5_cmd_apclone`, `c5_cmd_spectrum`, `c5_cmd_ciw` will appear broken until the C5 catches up.
- **c5_cmd.cpp:522-525** companion result accessors (`c5_stas`/`c5_probes`/`c5_deauth_hits`/`c5_spectrum_get`) return 0 forever — no storage. Anything reading from these silently sees empty results.

### 3. dhcp_cache memory bound

- **dhcp_cache.cpp:8** `DHCP_CACHE_MAX = 24`, hardcoded. Eviction by oldest-`last_seen` is implemented (:28-32), so unbounded growth is prevented. Bound is tiny — a busy LLMNR/responder run on a /24 with 30+ Windows hosts will churn. Acceptable for v0.6.x, flag for future tunable.
- **No thread guard.** `dhcp_learn` / `dhcp_try_parse_802_11` are called from promisc WiFi callbacks (per header comment line 17). The header claims "safe to call from an ISR or WiFi callback" but `dhcp_learn`'s `find()` + `s_n++` are not atomic. A second promisc callback racing on the same ISR could double-insert or skip eviction. Severity low (promisc cb runs on WiFi task, not real ISR; entries are 6+24 bytes so unaligned tearing risk is small) but the safety claim in the header is wrong.

### 4. net_attacks.cpp is large (906 LOC) — split candidate

Six independent features in one TU with shared helpers (`need_sta`, `probe_port`, `make_uuid`). Suggested split: `net_uart.cpp`, `net_tcp_tunnel.cpp`, `net_honeypot.cpp`, `net_deaddrop.cpp`, `net_printer.cpp`, `net_ssdp_poison.cpp`. Each <200 LOC. (Distinct from `net_ssdp.cpp`, which is the *discovery* scanner.)

### 5. Other large net files

- **net_dhcp.cpp 509 LOC** — `feat_net_hijack` (lines 421-509) inlines a near-copy of the rogue-DHCP loop. Should call `rogue_dhcp_loop(false)` with a duration parameter; currently 75 LOC of duplicate parsing.
- **net_wpad.cpp 556 LOC** — `build_type2` (lines 64-132) is shared between WPAD and Autodiscover but the WWW-Authenticate Type-3 readback (`feat_wpad_abuse`'s second-read pattern) appears twice (:249-261 and :460-475) and could be one helper.
- **net_cctv.cpp 588 LOC** — already cleanly factored; the three entry modes are short. No split needed.
- **c5_scan.cpp 967 LOC** — split candidate: `c5_status.cpp` (status/peers/dashboard), `c5_scan_wifi.cpp` (scan_5g + deauth + nuke), `c5_scan_zb.cpp` (zigbee), `c5_pmkid.cpp` (PMKID + handshake save helpers). All five entry points reuse `draw_status_header`.
- **mesh/meshtastic_node.cpp 591 LOC** — split candidate: `meshtastic_rx.cpp` (rx_task + handle_decoded_data), `meshtastic_tx.cpp` (mesh_tx_data + send_* + pack_header), `meshtastic_roster.cpp` (nodes + messages + snapshot/drain).

### 6. Meshtastic crypto / channel hardcoded

- **mesh/meshtastic_node.cpp:27-30** `DEFAULT_PSK[16]` is the LongFast default channel PSK, hardcoded. `MESH_CHANNEL_HASH` (=0x08, `meshtastic.h:49`) is also hardcoded. User-requested private channels cannot be configured — no NVS, no runtime API. `meshtastic.h:19-22` calls this out ("hardcoded to the default LongFast primary channel") but should surface a TODO in this audit: any private-channel rollout needs a multi-PSK store + per-channel `channel_hash` recompute.
- **mesh/meshtastic_node.cpp:464** `if (channel != MESH_CHANNEL_HASH) continue;` — any packet on another channel is silently dropped. If user joins a private mesh expecting traffic, the diagnostic will be "no messages" with no indication of the filter.

### 7. Credential file write paths

- **net_responder.cpp:185** `s_log = SD.open("/poseidon/ntlm.log", FILE_APPEND);` — APPEND, never rotates. Long-running responder run grows unbounded. No max-size check.
- **net_wpad.cpp:161-163** appends to `/poseidon/ntlm_hashes.txt` — same. Also: hash lines are emitted with `Serial.println` (line 164) which leaks plaintext to USB-CDC. Operator may not expect that.
- **net_wpad.cpp:438** `f.printf("BASIC|%s|%s|%s|%s\n", email, user, pass, domain)` — pipe-delimited but values aren't escaped. A username containing `|` corrupts the line.
- **net_lanrecon.cpp:259** `SD.open("/poseidon/lan.csv", FILE_WRITE)` — `FILE_WRITE` truncates on each run, so prior recon is overwritten silently. Should be APPEND with timestamp header (or use `sdlog_open` from sd_helper.h like net_cctv does at :322).
- **net_cctv.cpp:322** uses `sdlog_open("cctv", ...)` — correct timestamped file pattern. Good.
- **net_ssdp.cpp:158** `SD.open("/poseidon/ssdp.csv", FILE_APPEND)` writes header line every run — duplicate header rows accumulate across runs.

### 8. CCTV default-cred list

- **net_cctv.cpp:68-79** `DEFAULT_CREDS[]` is a hardcoded 10-entry table in flash. No SD override (Evil-M5 model would have user-extensible list). Either ship that capability or flag it as accepted scope cut.

### 9. SatCom TLE fetch — network timeout / cache invalidation

- **satcom.cpp:139** `(void)max_age_sec` — cache age intentionally ignored. Comment says "if you want new data, run the script and reflash." This is documented intent; flag as accepted but limits the "R" key refresh in `feat_satcom.cpp:131` to "live fetch only".
- **satcom.cpp:153** `http.setTimeout(8000)` — 8 s blocking. No abort path; user must wait. `feat_satcom.cpp:131` shows "refreshing TLE..." toast for 800 ms then the whole 8 s blocks the UI. Should be async (cooperative with `input_poll`) so ESC works.
- **satcom.cpp:155** `code != 200` returns false without logging — silently falls through to "No baked TLE" message in UI. Operator can't tell if it was a network failure vs a NORAD-not-in-favorites.

### 10. mesh ESP-NOW peer table — eviction race

- **mesh.cpp:120-124** evicts stale peers inside `mesh_task` by swap-with-last (`s_peers[i] = s_peers[--s_peer_count];`). Concurrent `on_recv` callback on the WiFi task can hit `find_peer` (read of `s_peer_count` without lock) during this swap. Result: read of half-moved entry or skipped index. Severity low (visual glitch in status screen) but the spec violation is real — unlike `c5_cmd.cpp` which uses `portMUX` correctly, `mesh.cpp` is lock-free with no atomics.
- **mesh.cpp:139** see defect 1 — `WiFi.getMode() == WIFI_OFF` test is wrong when raw-IDF is in use.

### 11. C5 ESP-NOW receive callback safety

- **c5_cmd.cpp:205** `on_recv` runs in the WiFi-task context (not a hard ISR but a high-priority task with limited stack). It calls `ensure_peer` → `esp_now_add_peer` which takes the ESP-NOW internal lock; if a UI thread is in `send_simple_cmd` and holds the same lock the recv may block briefly. Acceptable. More concerning: handlers call `Serial.printf` (`c5_cmd.cpp:191-194, 233-234, 376-385`) from this callback. USB-CDC writes can block for tens of ms; back-pressuring the WiFi rx queue. Move logs to a deferred ring (similar pattern as ble_db identifiers) for production builds.

### 12. GPS-off audit

- **mesh.cpp:100-105** wraps GPS inclusion behind `g.valid` — correct.
- **c5_scan.cpp** never tags WiFi captures with GPS. Wardrive-via-C5 is intentionally NOT in v3 proto (proto.h:11 "Wardrive intentionally NOT in this cut") — confirms gating.
- **feat_satcom.cpp:78-80** reads `gps_get()` for observer position. Read-only, never written to disk except via TLE cache file — no GPS leak.
- **mesh_position.cpp:62-65** position-reporting is *opt-in* via T key, default OFF (`s_position_reporting` initialized false at meshtastic_node.cpp:58). Correct.
- **No defect found** for GPS-off invariant in this bucket.

### 13. Misc

- **net_attacks.cpp:566** `dd_web = new WebServer(80);` allocates on a path that runs once per dead-drop entry, freed on exit. OK.
- **net_attacks.cpp:528** `static File upFile;` — function-static across HTTP requests (intentional, multi-chunk upload). Not reset between separate uploads; if a previous upload errored out without `UPLOAD_FILE_END`, the next upload's `UPLOAD_FILE_START` reopens and the old handle is leaked (overwritten without close). One-shot leak per failed upload.
- **net_lanrecon.cpp:182** `host_t &h = s_hosts[s_host_count]; ... s_host_count++` — `s_host_count` declared `volatile int` but assignment isn't atomic (read-modify-write). Not safe vs the ESC-poll path that reads s_host_count. Practically benign (single-writer pattern).
- **net_dhcp.cpp:159** `randomSeed(esp_random());` is a no-op on ESP32 (Arduino `random()` is already seeded from hardware RNG). Harmless.
- **net_responder.cpp:194** `s_mdns.listenMulticast(IPAddress(224,0,0,251), 5353);` answers mDNS — correctly fills the gap vs Evil-M5 and saltyjack which lack it.
- **net_dhcp.cpp:246-247** `rogue_client_t s_pool[ROGUE_POOL_SZ]` (32) — fine bound, but `s_pool_n` increments don't wrap; once 32 clients alloc'd, all further DHCP requests return suffix=0 and are dropped silently. UI shows "32/32" but no error toast.
- **c5_cmd.cpp:127-133** RESP_AP dedup-by-bssid only — if C5 reports the same BSSID on a different channel (e.g. dual-band 6 GHz colo), the second is dropped. Minor.
- **c5_cmd.cpp:344-348** `c5_peer_name()` returns pointer to live array without lock (line 347), while header at line 222 directs callers to `c5_peer_name_copy` for safety. The unsafe variant should be removed or marked deprecated.

## Cross-ref deltas

### vs Bruce
| Feature | POSEIDON | Bruce | Verdict | Notes |
|---|---|---|---|---|
| NTLMv2 responder | net_responder.cpp:176 + net_wpad.cpp | wifi/responder.cpp:113 | match | Bruce + POSEIDON both poison NetBIOS+LLMNR+SMB; POSEIDON also has mDNS. |
| Satellite tracker | satcom.cpp + feat_satcom.cpp | absent | poseidon-only | Bruce has no satcom. |
| ARP/MAC/DHCP wired | absent | ethernet/* | bruce-only | Bruce has W5500 wired LAN attacks. |
| LoRa chat | mesh stack (Meshtastic) | LoRaRF.cpp:401 lorachat | divergent | Bruce = point-to-point text. POSEIDON = full Meshtastic protocol (encrypted, roster, position, paging). POSEIDON is the deeper implementation. |
| Net portal HTTP | wifi_portal.cpp (out of scope) | evil_portal.cpp | match (bucket 1) | — |
| Net CCTV | net_cctv.cpp | absent | poseidon-only | — |
| Net SSDP discovery | net_ssdp.cpp | absent | poseidon-only | — |
| Mesh ESP-NOW | mesh.cpp PigSync + c5_cmd ESP-NOW | absent | poseidon-only | Bruce has no ESP-NOW mesh. |
| netcut (ARP poison) | absent | wifi/netcut.cpp:336 | bruce-only | Candidate import. |

### vs Ghost ESP
- **DIAL/Chromecast/YouTube cast hijack** (Ghost `dial_manager.c` + `dialconnect`) — POSEIDON lacks. *Flag as we_lack candidate.*
- **TP-Link Kasa smart-plug control** (Ghost `tplinktest`) — POSEIDON lacks. *Flag.*
- **Power Printer mass-print** (Ghost `powerprinter`) — POSEIDON's `feat_printer` only sends one job from SD `/poseidon/print.txt`. *We have a thin version; expansion candidate.*
- **WiFi LAN port-scan** — Ghost `scanports` cmd vs POSEIDON `feat_net_portscan` — same capability.
- **Stream-from-URL evil portal** — out of scope (bucket 1).
- **REST/SPA control surface** — POSEIDON's TRIDENT bridge is USB-CDC, not WiFi REST. Ghost serves a JSON REST API + 4 KB SPA. Architecture cut, not a defect.

### vs PORKCHOP
| Feature | POSEIDON | PORKCHOP | Verdict |
|---|---|---|---|
| ESP-NOW sync | mesh.cpp PigSync (open, no crypto) | pigsync_client.cpp (PMK+LMK encrypted) | porkchop-better |
| Net attacks | full LAN suite | absent | poseidon-only |
| Mesh stack | full Meshtastic + ESP-NOW | ESP-NOW only | poseidon-broader |
| Satcom | feat_satcom + SGP4 | absent | poseidon-only |
| Heap pressure mgmt | none in net_* | core/heap_health.h + EMA | porkchop-better — see Backlog |

### vs Evil-M5

Evil-M5 is the canonical LAN-attacks reference. Saltyjack is the explicit port (bucket 6). Within this bucket, POSEIDON's `net_*` family duplicates Evil-M5 functionality outside saltyjack/. Gaps:

| Evil-M5 capability | POSEIDON net_* status | Note |
|---|---|---|
| Captive cookie siphon (`/siphon`, `/logcookie`) | absent | net_attacks dead-drop has no JS siphon path. |
| Web admin / remote credential review | absent | No REST surface — only on-device UI. |
| SSDP fake/300-device farm | partial — net_attacks.cpp `feat_ssdp_poison` caps at 200 | Evil-M5 supports `MAX_SSDP_DEVICES=300` w/ PSRAM. POSEIDON broken-PSRAM unit can't match. |
| Switch DNS (AP↔STA bind toggle) | absent | Evil-M5 case 49 — net_responder.cpp could host. |
| Network Hijack combo (DHCP+DNS+WPAD chain) | partial — net_dhcp.cpp `feat_net_hijack` chains starve+rogue+portal but skips WPAD step | Should call `feat_wpad_abuse` between rogue DHCP and portal. |
| NTLM hash de-dup (`CleanNTLMHashes`) | absent | Evil case 62. Saltyjack also lacks. |
| ARP-scan targeting integration | net_lanrecon.cpp has ARP sweep but doesn't feed responder/wpad | Selective LLMNR target list could speed campaigns. |
| PCAP for net attacks | absent | All net_* writes plain CSV / NTLM-text. No `.pcap` for DHCP/Responder/WPAD flows. |
| mDNS poisoning | **POSEIDON has it** (net_responder.cpp:194) | Evil-M5 lacks; POSEIDON ahead. |
| LDAPDump anonymous bind | absent | Evil-M5 line 313. |
| IMSI catcher | absent | Out of scope (LTE). |

## Optimisation opportunities

- **Promisc-task heap.** `net_responder.cpp:201` xTaskCreate with 4096 B stack — over-provisioned for a connection-accept loop. 2048 B safe.
- **net_lanrecon.cpp:181** `Ping.ping(ip, 1)` per host with default 1 s timeout = up to 254 s for a /24. ARP-cache pre-warming via a single ARP request batch (`etharp_query` directly) would cut sweep to <30 s.
- **net_lanrecon.cpp:215** `c.setTimeout(250)` then `c.connect(...)` — `WiFiClient::connect` ignores `setTimeout`. Use `c.connect(ip, port, 250)` (already done in net_helpers.cpp:13 — recon should switch to `net_tcp_open`).
- **c5_cmd.cpp:380-385** `Serial.printf` for every peer send + every broadcast send. Spam on every tick of every C5 command. Wrap in `#ifdef C5_DEBUG` or rate-limit.
- **mesh/meshtastic_node.cpp:530** `xTaskCreatePinnedToCore(rx_task, "mesh_rx", 5120, nullptr, 3, ...)` — pinned to core 1 (good), but the RX task busy-polls `getPacketLength` every 20 ms (`meshtastic_node.cpp:449`). Could use SX1262 DIO IRQ via RadioLib's `setPacketReceivedAction` to fully sleep until packet — saves ~80% of cycles on this task.
- **net_attacks.cpp:528** `static File upFile` global — replace with `unique_ptr<File>` reset in `UPLOAD_FILE_START` so error paths close properly.
- **satcom.cpp:217** SGP4 STEP=30 s for 24 h window = 2880 iterations per pass-predict run; user blocks on this. Coarser 60 s step + refinement pass cuts compute ~50% without noticeable accuracy loss for AOS/LOS prediction.
- **net_cctv.cpp:452** for `scan_lan`, each host does 9 port probes serially = ~3 s/host worst case. Consider concurrent probes via 2-3 socket pool (TCP RX FD count permits).

## Refactor targets

1. **Split `net_attacks.cpp` 906 LOC** → six per-feature files. Shared `need_sta`/`probe_port` move to net_helpers.cpp.
2. **Split `c5_scan.cpp` 967 LOC** → status/scan-wifi/scan-zb/pmkid/nuke. `draw_status_header` + `auth_short` + `save_hs_to_sd` + `save_pmkid_to_sd` become local helpers in a new `c5_log.cpp`.
3. **Split `mesh/meshtastic_node.cpp` 591 LOC** → rx/tx/roster/lifecycle. Shared state moves to `meshtastic_internal.h`.
4. **Consolidate Type-2 build + Type-3 readback** between `net_wpad.cpp` and `net_responder.cpp` SMB stub. Single `ntlm_helpers.cpp` shared with saltyjack/.
5. **Replace `feat_net_hijack` inline rogue-DHCP duplicate** (`net_dhcp.cpp:458-491`) with parameterized `rogue_dhcp_loop(false, duration_ms)`.
6. **Add lock to `mesh.cpp` peer table** — port the `portMUX_TYPE s_mux` pattern from `c5_cmd.cpp`.
7. **Drop `c5_cmd.cpp` Serial.printf hot-path logging** behind a build flag.

## Backlog items

- **net-001** [HIGH] mesh.cpp:139 — `WiFi.getMode() == WIFI_OFF` test fails after raw-IDF init. Fix: copy `c5_cmd.cpp:248-253` `esp_wifi_get_mode` probe pattern. Effort: S.
- **net-002** [HIGH] net_attacks.cpp:557 / net_wpad.cpp:279 / net_wpad.cpp:512 / net_dhcp.cpp:327 — `WiFi.softAP(...)` (Arduino) violates [WiFi-AP-IDF] invariant. Fix: replace with raw `esp_wifi_set_config(WIFI_IF_AP, ...) + esp_wifi_start()` per recipe in `feedback_poseidon_wifi_ap_recipe`. Effort: M (4 sites, identical pattern).
- **net-003** [HIGH] mesh.cpp:120-124 — lock-free peer eviction races recv cb on WiFi task. Fix: wrap `s_peers`/`s_peer_count` ops in `portMUX_TYPE` like c5_cmd. Effort: S.
- **net-004** [HIGH] satcom.cpp:153 — 8 s blocking `http.GET` freezes UI; ESC ignored. Fix: replace HTTPClient with chunked async loop polling `input_poll` between socket reads. Effort: M.
- **net-005** [MED] c5_scan.cpp 967 LOC — split candidate. Fix: extract `c5_status.cpp`, `c5_scan_wifi.cpp`, `c5_scan_zb.cpp`, `c5_pmkid.cpp`, `c5_log.cpp`. Effort: M.
- **net-006** [MED] net_attacks.cpp 906 LOC — split candidate. Fix: per-feature files. Effort: M.
- **net-007** [MED] mesh/meshtastic_node.cpp 591 LOC — split candidate (rx / tx / roster / lifecycle). Effort: M.
- **net-008** [MED] mesh/meshtastic_node.cpp:27 — hardcoded LongFast PSK; no private-channel support. Fix: per-channel PSK + hash via NVS-backed `mesh_channel_t` struct. Effort: L.
- **net-009** [MED] net_responder.cpp:185 / net_wpad.cpp:161 — credential file append unbounded; no size cap, no rotation. Fix: roll at 64 KB to `.1`, `.2`, ...; cap 4 files. Effort: S.
- **net-010** [MED] net_wpad.cpp:164 — `Serial.println("[+] NTLMv2: " + line)` leaks captured hash to USB-CDC. Fix: gate behind `POSEIDON_DEBUG_NTLM`; default off. Effort: S.
- **net-011** [MED] net_lanrecon.cpp:259 — `FILE_WRITE` truncates `/poseidon/lan.csv` each run. Fix: switch to `sdlog_open("lan", ...)` like net_cctv does. Effort: S.
- **net-012** [MED] net_responder.cpp:194 — handler-direct response from ESP-NOW-adjacent callback (AsyncUDP). AsyncUDP runs its own task; the responder writes back into it inside `on_*` — fine. But same handler does `s_log.printf` + `s_log.flush` on every query which can block 5-20 ms (SD I/O). Fix: queue-defer log lines to a worker. Effort: M.
- **net-013** [MED] c5_cmd.cpp:380-385 — Serial.printf hot-path logging in `send_simple_cmd` every call. Fix: gate behind `C5_DEBUG`. Effort: S.
- **net-014** [LOW] c5_cmd.cpp:522-525 — `c5_stas`/`c5_probes`/`c5_deauth_hits`/`c5_spectrum_get` return 0 forever (no storage). Either implement ring buffers or remove the API surface until C5 firmware lands. Effort: M.
- **net-015** [LOW] dhcp_cache.cpp:22 — header claims ISR-safe but `dhcp_learn` is not atomic. Fix: either add `portMUX` or correct the header comment. Effort: S.
- **net-016** [LOW] satcom.cpp:139 — `max_age_sec` parameter ignored. Fix: drop the parameter or implement sidecar file with `fetched_ts`. Effort: S.
- **net-017** [LOW] net_attacks.cpp:528 — `static File upFile` leaks if upload errors before `UPLOAD_FILE_END`. Fix: close pre-existing handle on `UPLOAD_FILE_START`. Effort: S.
- **net-018** [LOW] net_attacks.cpp:744 — SSDP poisoner cap 200 devices (Evil-M5 is 300). Fix: tune up if PSRAM detected (broken on this unit — leave at 200). Effort: S.
- **net-019** [LOW] net_dhcp.cpp:246 — `ROGUE_POOL_SZ=32` silent ceiling, no UI warning when reached. Fix: toast at 90%. Effort: S.
- **net-020** [LOW] net_ssdp.cpp:158 — duplicate header lines accumulate across appends. Fix: only write header if file is empty (use `f.size() == 0` check). Effort: S.
- **net-021** [LOW] mesh/meshtastic_node.cpp:449 — RX task polls every 20 ms; could be IRQ-driven via `setPacketReceivedAction`. Effort: M (radiolib API change risk).
- **net-022** [LOW] c5_cmd.cpp:344-348 — `c5_peer_name()` race-prone; `c5_peer_name_copy()` is the safe one. Fix: mark unsafe variant `[[deprecated]]` or delete and chase callers. Effort: S.
- **net-023** [LOW] net_lanrecon.cpp banner+http use raw `WiFiClient` instead of `net_helpers::net_http_get` / `net_tcp_open`. Fix: route through helpers. Effort: S.
- **net-024** [WE_LACK] Ghost ESP DIAL/Chromecast hijack (`dialconnect`) — useful LAN feature. Fix: port `dial_manager.c` → `net_dial.cpp`. Effort: L.
- **net-025** [WE_LACK] Ghost TP-Link Kasa control (`tplinktest`) — small XOR-encoded protocol. Fix: port → `net_kasa.cpp`. Effort: M.
- **net-026** [WE_LACK] Evil-M5 cookie siphon (`/siphon`, `/logcookie`) — pair with dead-drop or portal. Effort: M.
- **net-027** [WE_LACK] Evil-M5 NTLM hash de-dup pass — also missing in saltyjack. Fix: shared `ntlm_dedup_file()` helper. Effort: S.
- **net-028** [WE_LACK] PCAP writer for DHCP / Responder / WPAD flows (per Evil-M5). Fix: add `pcap_writer.cpp` shared with bucket-1 captures. Effort: L.
- **net-029** [WE_LACK] Bruce netcut (ARP poison via LwIP linkoutput). Fix: `net_netcut.cpp`. Effort: M.
- **net-030** [INFO] proto v3 work-in-progress — C5-side handlers for opcodes 18-26 still missing; result accessors 46-49 unwired. Track in c5_node board, not here. Effort: tracking only.
