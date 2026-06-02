# WiFi Module Audit

## Current state

The WiFi bucket spans 5,394 LOC across 15 .cpp + 3 .h files plus an inline
shared frame builder. Entry points: WiFi Scan `wifi_scan.cpp:340` (raw IDF
2-pass scan, C5 5GHz merge, seeds `g_last_selected_ap`); Targeted Deauth
`wifi_deauth.cpp:205` (promisc client harvest + 4-frame bursts via
`wifi_deauth_pair`); Broadcast Deauth `wifi_deauth_extras.cpp:125` ("nuke
all" with C5 5GHz delegation) plus Deauth Detector `:289`; Beacon Spam
`wifi_beacon_spam.cpp:146` (meme/rickroll/custom SSID list, raw 802.11
beacons at 10Hz); Evil Portal `wifi_portal.cpp:582` (raw-IDF AP + 16
HTML templates from `wifi_portal_extras.h` + DNS wildcard); AP Clone
`wifi_portal.cpp:653`; Evil Twin `evil_twin.cpp:364` (time-sliced
deauth/portal phases, 5s/25s); Wardrive `wifi_wardrive.cpp:180`
(channel-hop beacon logger → WiGLE 1.6 CSV, seeds `g_wdr_aps`); PMKID +
4-way Handshake `wifi_pmkid.cpp:458` (hashcat 22000, M1/M2 cache, hunt
mode); Probe Sniff + Karma `wifi_probe.cpp:516/517` (probe-response
injection + background beacon spam); Clients-for-AP `wifi_clients.cpp:98`;
Global Client Hunter `wifi_clients_all.cpp:148`; Spectrum
`wifi_spectrum.cpp:385` (3 styles: bars / waterfall / radar); CIW
`wifi_ciw.cpp:214` (157-payload SSID injector); Sanity Override
`wifi_sanity_override.cpp:33` (linker-strong stub of
`ieee80211_raw_frame_sanity_check` returning 0, exposes
`wifi_deauth_last_rc`); AP Signal Test `ap_signal_test.cpp:144`
(diagnostic POSEIDON-SIGTEST broadcast).

Shared frame builder `wifi_deauth_frame.h` (254 LOC, inline) defines
`wifi_silent_ap_begin` (STA-only + promisc MASK_ALL + channel lock — no
real softAP since the sanity override is in place), `_deauth_build`
(26-byte frame), `wifi_deauth_pair` (4 frames per burst, rotating reason
codes, 2ms vTaskDelay between frames), `wifi_deauth_broadcast`, and
`wifi_auth_has_pmf` (WPA3/Enterprise hint).

Raw-IDF AP recipe (`esp_wifi_init` w/ shrunk bufs +
`esp_netif_create_default_wifi_ap` + `esp_wifi_set_config(WIFI_IF_AP)` +
`esp_wifi_set_channel` AFTER `esp_wifi_start` + 1500ms settle) is
followed correctly in `wifi_portal.cpp:395-481`, `evil_twin.cpp:183-226`,
`ap_signal_test.cpp:38-116`. BTDM mem release (one-way, kills BLE for
session) is invoked by `wifi_portal.cpp:430`, `evil_twin.cpp:401`,
`ap_signal_test.cpp:62`.

Background tasks: 11 `xTaskCreate` sites — most at 3072B stack, prio 4.
Wardrive + PMKID + global-clients + spectrum + beacon-spam +
deauth-extras + targeted-deauth + scan-task each spawn one hop/sniff
task. Concurrency primitives: `portMUX_TYPE` spinlocks in wardrive,
deauth, probe — PMKID intentionally does NOT lock (RX task is
single-threaded, blocking SD writes would deadlock under ISR critical).

## Defects

1. **[CRIT]** `wifi_ciw.cpp:238-239,265,295-296` — Uses Arduino
   `WiFi.mode(WIFI_MODE_AP)` + `WiFi.softAP()` which is documented to
   crash `ieee80211_hostap_attach +0x2c` under pinned Bruce libs
   (`wifi_portal.cpp:396`, `evil_twin.cpp:23`, `ap_signal_test.cpp:6`).
   Single active user of the forbidden Arduino AP path. **Fix:** replace
   with the raw-IDF recipe from `wifi_portal.cpp:417-481`; reuse the
   beacon-only `s_beacon` raw-TX path for SSID rotation instead of
   re-attaching softAP every cycle.

2. **[CRIT]** `wifi_ciw.cpp:265` — Re-calling `WiFi.softAP()` every
   `rotateInterval` ms tears down + re-attaches the AP TX state in-loop;
   on Bruce libs this is the documented thrash pattern that causes
   `ESP_ERR_NO_MEM (257)` + eventual `LoadProhibited 0x2c`. **Fix:**
   raw-IDF AP up ONCE, mutate SSID via `esp_wifi_set_config(WIFI_IF_AP,...)`
   or rotate only via raw beacon TX (subtype 0x80), never re-softAP.

3. **[HIGH]** `wifi_ciw.cpp:296` — Teardown sets
   `WiFi.mode(WIFI_MODE_APSTA)` which doubles WiFi static+dynamic
   buffers (~30KB heap pressure per `wifi_deauth_frame.h:58-61`) and
   leaves the next radio user in a corrupted state. **Fix:** call
   `esp_wifi_stop(); esp_wifi_deinit();` matching the raw-IDF teardown
   used elsewhere; never end in APSTA.

4. **[HIGH]** `wifi_portal.cpp:430` + `evil_twin.cpp:401` +
   `ap_signal_test.cpp:62` — `esp_bt_controller_mem_release(ESP_BT_MODE_BTDM)`
   is called every entry; one-way until power cycle. Triton already
   removed this (`triton.cpp:1027` comment). User has no way to recover
   BLE without unplug. **Fix:** check
   `esp_bt_controller_get_status()` first and only release if BLE was
   never inited this session, or document the cost in the menu entry's
   info string; consider gating behind a "kill BLE for session" prompt.

5. **[HIGH]** `wifi_portal.cpp:539-540` + `evil_twin.cpp:329-330` —
   `s_dns->processNextRequest()` and `s_http->handleClient()` are called
   synchronously inside the UI loop with `delay(5)` between iterations.
   During the `ui_action_overlay(... 1200)` blocking call at
   `:551`/`:337`, DNS+HTTP service halts for 1.2s — captive-portal
   probe phones may time out and abort the redirect. **Fix:** spawn a
   dedicated `xTaskCreatePinnedToCore` for DNS+HTTP or run the overlay
   as a non-blocking state machine.

6. **[HIGH]** `wifi_pmkid.cpp:447-449` — `draw_notification` calls
   `M5Cardputer.Speaker.tone()` + `delay(130)` + `tone` + `delay(130)`
   while the WiFi RX task is firing `promisc_cb` → `emit_handshake` →
   blocking SD writes. UI main loop blocks for ~600ms, RX task continues
   queueing into the dynamic_rx_buf pool which is sized at 8 — under
   sustained EAPOL traffic this can overflow before the UI returns.
   **Fix:** non-blocking tone sequence (state machine) or move
   notification to a dedicated low-prio task.

7. **[HIGH]** `wifi_scan.cpp:531` — Rescan `xTaskCreate` uses 8192B
   stack. Wifi.scanNetworks runs inline in the task and the lib's own
   wifi callback dispatch can stack ~2-3KB. Stack pressure on rescan is
   real — investigate whether the `cached` rescan path actually returns
   to the inline scan path or actually uses this task; if used, stack
   canary risk remains. **Fix:** verify via runtime canary check or
   collapse rescan to the inline path used at `:357-479` (which the
   author already noted at `:354-356` solved a canary overflow).

8. **[HIGH]** `wifi_ciw.cpp:222-228` — `static CiwPayload active[PAYLOAD_COUNT]`
   in BSS at 64+1 bytes × ~157 = ~10KB. Large fixed BSS competes with
   WiFi driver allocations after `esp_wifi_init`. Combined with the
   AP-softAP path this is a memory pressure point. **Fix:** allocate
   into heap on entry, free on exit, or shrink ssid field to 33 bytes
   (max valid SSID len + nul) — saves ~5KB.

9. **[MED]** `wifi_beacon_spam.cpp:40` — Global non-const
   `static uint8_t s_beacon[128]` is mutated from `spam_task` while
   user-space `pick_list` UI code reads from main task. No
   synchronization. Race is benign in practice (only seen during
   transitions) but is a sharp edge. **Fix:** allocate frame on the
   task's stack, or `portMUX` the mutation, or document why race is
   acceptable.

10. **[MED]** `wifi_deauth_extras.cpp:34` — `broad_task` calls
    `wifi_silent_ap_begin(1)` ONCE at task entry then iterates targets
    with `esp_wifi_set_channel`. Different from
    `wifi_clients_all.cpp:123,138` which calls `wifi_silent_ap_begin`
    per-burst. Inconsistency; document or unify. **Fix:** standardize on
    one pattern (begin-once is correct given STA-only model).

11. **[MED]** `wifi_clients.cpp:90` — `deauth_client` calls
    `wifi_silent_ap_begin(s_target_ch)` + 30 pairs synchronously while
    on the main UI task. With 4 frames × 30 iter × ~8ms ≈ 1s of UI
    freeze. **Fix:** spawn a brief task for the burst, or drop iteration
    count + spread bursts across UI cooperative ticks.

12. **[MED]** `wifi_probe.cpp:222-240` — `probe_cb` (WiFi RX task ctx)
    calls `esp_wifi_80211_tx` directly. While IDF docs say this is safe,
    it serializes RX with TX inside the WiFi task; sustained high probe
    traffic could starve RX. **Fix:** queue (sta+ssid) pairs into a
    StreamBuffer and TX from a dedicated low-prio task; pattern matches
    Karma's beacon-spam loop at `:419-437`.

13. **[MED]** `wifi_portal.cpp:497-501` + `evil_twin.cpp:309-313` —
    `new DNSServer` and `new WebServer(80)` heap-allocate every portal
    cycle. Evil-twin churns this every 30s, fragments heap. **Fix:**
    keep file-scope statics and only `begin()`/`stop()`, or move to
    arena allocator.

14. **[MED]** `wifi_pmkid.cpp:149,152,170,177` — `s_emit_handshake_line[600]`
    is file-scope BSS. Comment at `:142-148` notes the trade-off vs
    stack overflow; correct call. However handshake hex blob can
    theoretically exceed 600B if M2 EAPOL packet body is > 260B (RSN
    KDE with multiple AKMs). **Fix:** add `if (m2_len > 260) m2_len=260`
    guard before `hex_append` at `:174-175`.

15. **[MED]** `wifi_pmkid.cpp:319-336` — `promisc_cb` deliberately omits
    `portMUX` but `s_m1[]` and `s_cache[]` writes here race with reads
    from `hunt_task` (`:362-381`). hunt_task only reads `s_cache[i].bssid`
    while `cache_beacon` may be modifying the same slot. **Fix:** snapshot
    BSSID list under brief crit-section in `hunt_task` before the burst
    loop, or add atomic seqlock-style versioning.

16. **[MED]** `wifi_wardrive.cpp:53-67` — `wdr_open_csv` writes
    `g.utc[0]='\0'; g.date[0]='\0'` to placeholder fields when GPS
    invalid, but `flush_dirty_rows:97-98` writes `g.date` directly to
    CSV row when GPS comes online mid-session. Row rewriting on later
    flush mixes valid-GPS rows with placeholder timestamps. **Fix:**
    keep first-seen timestamp + GPS-at-first-fix per AP, don't conflate
    with current snapshot.

17. **[MED]** `wifi_wardrive.cpp:160-165` — GPS lat/lon/alt only written
    when `rssi > a.rssi || a.rssi == 0`. If first sighting has no GPS
    fix but a stronger sighting comes later WITH a fix, GPS is updated;
    but if a weaker later sighting has GPS while the strongest never
    did, GPS stays zero. WiGLE row will have `0.0,0.0` coords. OPSEC
    invariant violation (writes 0,0 even when GPS never had fix).
    **Fix:** gate GPS write on `g.valid` AND only write coords when
    fix is real; emit empty fields for unfixed rows.

18. **[MED]** `wifi_ciw.cpp:281-286` — `ui_text` then `%.33s` on a 34
    char buf where SSID field is 64B. Truncates display only; actual
    beacon TX uses full 64B at `:166-169` (capped to 32 in `build_beacon`).
    Defects: CIW payload header allocates 64B per SSID but only first
    32B reaches air — second half of payloads (those > 32 chars) are
    silently truncated. **Fix:** drop ssid field size to 33B (32 + nul)
    OR build longer SSID into a real 802.11 SSID tag (IDs > 32 are
    invalid per spec but interesting fuzz vector).

19. **[MED]** `wifi_deauth.cpp:38,42,44,304-305` — `s_target`,
    `s_channel`, `s_seq` are file-scope but `s_deauth_task_alive`
    serializes resume. The `s_seq` is read+written from both
    `deauth_task` and `wifi_deauth_pair` (which does `(*seq)++`); race
    on resume window (200ms gap between old task exit and new task
    start) means stale `s_seq` if `deauth_task` exits between paint and
    resume. Likely benign. **Fix:** atomic seq counter or document.

20. **[MED]** `wifi_clients_all.cpp:118-131` — `unicast_deauth` toggles
    `s_locked` to suppress the hop task during burst. But hop_task only
    checks `s_locked` once per loop iter (`:97-103`), with a 200/400ms
    delay; a deauth burst can fire across a channel hop boundary. **Fix:**
    use a binary semaphore or join hop_task to a state machine.

21. **[MED]** `wifi_portal.cpp:539-540,572` — Tight loop with `delay(5)`
    inside `if (k == PK_NONE)` but UI redraw is gated at 250ms. Means
    when no input, loop runs 50 iter/250ms ≈ 200 `processNextRequest`
    + `handleClient` calls per UI frame. CPU thrash on idle.
    **Fix:** raise delay to 20ms when no clients connected (check
    `esp_wifi_ap_get_sta_list`), 5ms otherwise.

22. **[LOW]** `wifi_portal.cpp:434-435` + `evil_twin.cpp:138-139` +
    `ap_signal_test.cpp:66-67` — `esp_netif_init` + `esp_event_loop_create_default`
    called every entry. Both are idempotent (return ESP_ERR_INVALID_STATE
    on second call) but rc is ignored. **Fix:** call from boot once,
    gate with a static bool.

23. **[LOW]** `wifi_beacon_spam.cpp:149-150` — `WiFi.mode(WIFI_STA)`
    Arduino call (not the forbidden softAP path, but still uses the
    Arduino layer that flags above). Documented working but
    inconsistent with the lean IDF init pattern. **Fix:** use
    `wifi_lean_sta_init()` for consistency.

24. **[LOW]** `wifi_spectrum.cpp:388` — Same Arduino `WiFi.mode(WIFI_STA)`
    inconsistency. **Fix:** use `wifi_lean_sta_init()`.

25. **[LOW]** `wifi_scan.cpp:67-79` — Dead STA-mode retry loop in
    `scan_task` (Rescan only). **Fix:** collapse with wifi-010.

26. **[LOW]** `wifi_deauth_extras.cpp:55` — `s_b_sent += 1500` estimated
    per C5 5GHz rotation inflates the displayed deauth-per-sec.
    **Fix:** separate "C5 5G in flight" badge instead.

27. **[LOW]** Promiscuous cb dangling on exit at
    `wifi_pmkid.cpp:507`, `wifi_deauth.cpp:243`,
    `wifi_wardrive.cpp:217`, `wifi_spectrum.cpp:395`,
    `wifi_clients.cpp:126`, `wifi_clients_all.cpp:164`,
    `wifi_deauth_extras.cpp:300`. **Fix:** add
    `esp_wifi_set_promiscuous_rx_cb(nullptr)` before promisc-off in
    every exit path.

## Cross-ref deltas

### vs Bruce

| Feature | POSEIDON | Bruce | Verdict | Notes |
|---------|----------|-------|---------|-------|
| WiFi scan | `wifi_scan.cpp:340` | `wifi_common.cpp:183` | we_have_better | POSEIDON does raw IDF scan + WPS IE parse + C5 5GHz merge; Bruce is Arduino `scanNetworks` only |
| WiFi deauth | `wifi_deauth.cpp:205` | `deauther.cpp:143` | divergent | POSEIDON: STA-iface raw TX + linker-override; Bruce: AP-iface TX with `esp_wifi_80211_tx(..., false)` en_sys_seq trick. Both land but POSEIDON's path is more memory-efficient. |
| Broadcast deauth | `wifi_deauth_extras.cpp:125` | not present | we_have_better | Bruce has targeted only |
| WiFi beacon spam | `wifi_beacon_spam.cpp:146` | `wifi_atks.cpp:977` | parity | both raw 802.11; Bruce ships HE capabilities IE |
| WiFi probe | `wifi_probe.cpp:516` | folded in Karma `karma_attack.cpp:909` | we_have_better | standalone probe sniffer |
| WiFi Karma | `wifi_probe.cpp:517` (run_karma) | `karma_attack.cpp:909` | parity | both real Karma (probe-response in cb); Bruce has more elaborate state machine |
| Evil Portal | `wifi_portal.cpp:582` | `evil_portal.cpp:61` | divergent | POSEIDON: 16 inline HTML templates + raw-IDF AP; Bruce: AsyncWebServer + 1 template (SD-loadable) |
| Evil Twin | `evil_twin.cpp:364` | not standalone | we_have_better | Bruce doesn't time-slice deauth+portal |
| AP Clone | `wifi_portal.cpp:653` | not present | we_have_better | n/a |
| Wardrive | `wifi_wardrive.cpp:180` | `wardriving.cpp:451` | parity | both WiGLE 1.6 CSV; Bruce has wigle.net upload |
| PMKID | `wifi_pmkid.cpp:458` | counter in `karma_attack.cpp:2326` | we_have_better | Bruce has no dedicated PMKID extractor |
| Handshake | `wifi_pmkid.cpp:151` (full M1/M2) | `wifi_recover.cpp:235` (PCAP parse) | we_have_better | live capture vs offline parse |
| Clients (one AP) | `wifi_clients.cpp:98` | `sniffer.cpp:696` | parity | both promisc + STA harvest |
| Clients (all) | `wifi_clients_all.cpp:148` | not present | we_have_better | hop+global hunt is unique |
| Spectrum (WiFi) | `wifi_spectrum.cpp:385` | not present | we_have_better | n/a |
| Sanity override | `wifi_sanity_override.cpp:33` | `wifi_atks.cpp:157` `en_sys_seq=false` | divergent | POSEIDON: linker stub; Bruce: per-call flag. POSEIDON's is more robust on stock blobs. |
| Deauth detector | `wifi_deauth_extras.cpp:289` | not present | we_have_better | n/a |
| CIW SSID injection | `wifi_ciw.cpp:214` | not present | we_have_better | n/a |
| AP signal test | `ap_signal_test.cpp:144` | not present | we_have_better | n/a |

### vs Ghost ESP

| Feature | POSEIDON | Ghost ESP | Verdict | Notes |
|---------|----------|-----------|---------|-------|
| WiFi scan | `wifi_scan.cpp:340` | `wifi_manager.c:944` | we_have_better | Ghost lacks WPS IE flag |
| WiFi deauth | `wifi_deauth.cpp:205` | `wifi_manager.c:1099` | parity | Ghost: 26-byte deauth+disassoc template + ch hop 1-11 task; POSEIDON: same template + per-AP STA harvest |
| Beacon spam | `wifi_beacon_spam.cpp:146` | `wifi_manager.c:2153` | we_lack | Ghost ships 802.11ax HE Capabilities IE for Wi-Fi 6 spoofing; POSEIDON doesn't |
| Beacon AP-list mode | not present | `wifi_manager_broadcast_ap` `-l` flag | we_lack | Re-broadcast scanned APs |
| Evil portal | `wifi_portal.cpp:582` | `wifi_manager.c:726` | divergent | Ghost: URL-proxied HTML; POSEIDON: inline templates. Ghost simpler ops, POSEIDON works offline |
| Wardrive | `wifi_wardrive.cpp:180` | `callbacks.c:505` | parity | both WiGLE CSV |
| EAPOL capture (PCAP) | not present | `callbacks.c:639` | we_lack | Ghost writes pcap; POSEIDON only writes hashcat 22000. Both formats are useful |
| Pwnagotchi detect | not present | `callbacks.c:626` | we_lack | n/a |
| WPS detect | `wifi_scan` flags WPS IE | `callbacks.c:652` capture | parity | POSEIDON surfaces in scan UI; Ghost captures to pcap |
| PineAP detect | not present | `callbacks.c:174/192/307` | we_lack | Rogue-AP heuristic |
| Sanity override | `wifi_sanity_override.cpp:33` | `main/main.c:23` | parity | identical linker-stub trick; Ghost's lives in `main.c`, ours in dedicated TU |
| Auto-deauth (boot) | not present | `wifi_manager.c:2006/2074` `wifi_auto_deauth_task` | we_lack | Flipper-style continuous on USB build |
| LAN port scan | not in WiFi bucket (net_*) | `wifi_manager.c:1633` | parity | both have; ours separate file |
| AP REST control | not present | `ap_manager.c:846` (`/api/*`) | we_lack | Worth considering for headless ops |

### vs PORKCHOP

| Feature | POSEIDON | PORKCHOP | Verdict | Notes |
|---------|----------|----------|---------|-------|
| WiFi scan | `wifi_scan.cpp:340` | `network_recon.cpp:759` | divergent | POSEIDON: 2-pass active; Porkchop: promisc beacon harvest (continuous) |
| WiFi deauth | `wifi_deauth.cpp:205` | `oink.cpp` (sendDeauthFrame) | parity | both bypass sanity check; both use STA iface |
| Sanity bypass | `wifi_sanity_override.cpp:33` | `wsl_bypasser.cpp` + `-Wl,-zmuldefs` in `platformio.ini:34` | parity | EXACT same linker technique. POSEIDON should verify `-zmuldefs` is in our `platformio.ini` per memory invariant. |
| Beacon spam | `wifi_beacon_spam.cpp:146` | `bacon.cpp:496` (BACON game) | divergent | Porkchop's is a game protocol (Vendor IE OUI 0x50:52:4B); POSEIDON's is denial/troll spam |
| Hidden-SSID reveal | not present | `spectrum.cpp` | we_lack | broadcast deauth → probe-resp dump |
| Wardrive | `wifi_wardrive.cpp:180` | `warhog.cpp:459` | parity | both WiGLE; Porkchop has Sirloin bounty list |
| PMKID | `wifi_pmkid.cpp:458` | `oink.cpp` processEAPOL | parity | both EAPOL + hashcat 22000 |
| Handshake (M1+M2) | `wifi_pmkid.cpp:151` | `oink.cpp` processEAPOL | parity | both formats; Porkchop also writes PCAP |
| BOAR BROS exclusion list | not present | `oink.h:172` 50-entry fixed | we_lack | wardrive exclusion list with persistence — useful |
| Heap-pressure gates | not present | `heap_health.h`, `heap_gates.h` | we_lack | Tiered state machine — should consider |
| Reservation fence | not present | `main.cpp:59-87` 80KB fence | we_lack | Pre-allocate then free to push WiFi driver buffers high. Worth backporting for fragmentation. |
| WiFi.mode(OFF) ban | not enforced | `wifi_utils.cpp:227-229` | we_lack | Porkchop explicitly warns against `WIFI_OFF`; POSEIDON's `wifi_ciw.cpp:296` ends in `APSTA` which is the worse pattern |
| WiFi+BLE coexistence sequence | partial (BTDM release one-way) | `network_recon.cpp:794-822` strict ordering | we_lack | Porkchop has documented teardown sequence; POSEIDON sledgehammer-releases BTDM |
| Single cooperative loop | partial (Triton style) | all attack modes via `update()` | divergent | POSEIDON uses xTaskCreate per feature (11 sites in WiFi alone); Porkchop sidesteps the rc=-1 silent-fail by avoiding tasks |
| Hidden adaptive DNH hop | not present | `donoham.cpp` 4-state | we_lack | Per-ch dead-streak hopping |

### vs Evil-M5

| Feature | POSEIDON | Evil-M5 | Verdict | Notes |
|---------|----------|---------|---------|-------|
| WiFi scan | `wifi_scan.cpp:340` | inline `WiFi.scanNetworks` | we_have_better | raw IDF + WPS |
| Captive portal | `wifi_portal.cpp:582` | `:3861-3892` AsyncWebServer + 31 SD templates | divergent | POSEIDON: 16 inline; Evil-M5: 31 base + 27 community templates from SD `/evil/sites/` |
| Portal template count | 16 inline | 58 (31 base + 27 community) | we_lack | many brand/locale templates not present |
| Portal SD-loadable | NO | YES (`/evil/sites/*.html`) | we_lack | operator can't drop in templates without rebuild |
| Cookie siphon | not present | `:3895 /siphon /logcookie` | we_lack | n/a |
| Evil Twin | `evil_twin.cpp:364` | `:20154 startEvilTwin` | parity | both clone+portal+deauth |
| Karma | `wifi_probe.cpp:517` | `:2373 StartScanKarma` FSM | parity | Evil has 3 sub-modes (Karma/AutoKarma/Spear); POSEIDON has 1 |
| AutoKarma rotate ch | not present | `karmaChannels[]=1,6,11` | we_lack | n/a |
| Beacon spam | `wifi_beacon_spam.cpp:146` | `case 22` from `CustomBeacons` config | parity | Evil's list is SD-config'd |
| Deauth | `wifi_deauth.cpp:205` | `:10829 deauthAttack` | parity | both raw 802.11 |
| Auto-deauth + pwn detect | not present | `case 24` | we_lack | n/a |
| Sniff+deauth clients | `wifi_clients.cpp:98` | `case 30 deauthClients` | parity | both EAPOL-window after deauth |
| Handshake/PMKID | `wifi_pmkid.cpp:458` | pcap-based, multi-HS parser | divergent | Evil writes PCAP; POSEIDON writes hashcat 22000 |
| Multi-HS per PCAP | n/a (no PCAP) | `MAX_HS_PER_PCAP=12` | we_lack | (no equivalent, format different) |
| Wardrive | `wifi_wardrive.cpp:180` | helpers in `utilities/wardriving/` | parity | both WiGLE |
| Captive DNS | `wifi_portal.cpp:498-499` wildcard `*` | `:225 DNSServer "*"` | parity | identical pattern |
| BadUSB / drone / NTLM / WPAD / DHCP | not in WiFi bucket | Evil's case 60/71/77/78/etc. | n/a | in other buckets |

## Optimisation opportunities

1. `wifi_scan.cpp:531` — Rescan task at 8192B stack; if collapsed to
   inline scan (current default path), saves 8KB peak stack + the
   task-create overhead. Expected gain: more reliable scan under heap
   pressure.

2. `wifi_deauth_frame.h:191-197` — `wifi_deauth_pair` does 4×
   `esp_wifi_80211_tx` + `vTaskDelay(2ms)` per pair. Total 8ms airtime
   per pair. Bruce/Porkchop typically do 1 frame per pair with longer
   reason cycling. Consider dropping STA→AP reverse direction in
   bandwidth-constrained scenarios (broadcast deauth on busy AP) —
   halves airtime. Expected gain: ~2× deauth-per-second throughput.

3. `wifi_portal.cpp:557` + `evil_twin.cpp:346` — UI redraw gated at 250ms
   with full `fillRect` + multiple `printf`. Status fields change rarely
   (clients/hits/creds). Use dirty-tracking like `wifi_scan.cpp:486-503`
   to skip identical paint. Expected gain: reduce TX-cache stall risk
   during AP active periods.

4. `wifi_spectrum.cpp:241-247` — Waterfall pushes new row every 80ms but
   re-paints all `draw_rows × 13` cells every frame, even unchanged ones.
   Roll the ring buffer in display memory via `M5Cardputer.Display.scroll`
   if supported by M5GFX, otherwise track per-cell dirty. Expected gain:
   smoother waterfall + lower TX-cache stall risk during sniff bursts.

5. `wifi_pmkid.cpp:88-91` — `hex_append` uses `strlen` + `sprintf` per
   call; called 6× per emit. Replace with manual nibble→char loop.
   Expected gain: handshake emit latency drop from ~5ms to <1ms — frees
   SD bus for the next EAPOL.

6. `wifi_portal_extras.h` — 395 LOC of HTML in rodata. Compressing with
   miniz at build time (custom build step) or moving to LittleFS
   partition reclaims ~30KB flash. Expected gain: more app space for
   features.

7. `wifi_beacon_spam.cpp:99` — `delay(30)` between beacon TXs gives 33Hz
   per-SSID broadcast. Across 20 meme SSIDs that's 1.65s per SSID
   rotation; modern phones probe-scan every 1-2s, can miss SSIDs.
   Drop to `delay(15)` if TX pool can sustain. Expected gain: 2×
   probability of scan picking up all spammed SSIDs.

8. `wifi_ciw.cpp:222` — Move `active[]` from BSS to heap; saves ~10KB
   permanent BSS that competes with WiFi driver. Expected gain:
   reduce `wifi_init` rc=257 risk on cold-boot from heap-fragmented
   state.

9. `wifi_clients.cpp:182-204` + `wifi_clients_all.cpp:197-230` — Per-row
   draw does 5+ `printf`+`setCursor` per visible row × 7-8 rows × every
   300ms-1s. Cache row hash, only redraw changed rows. Expected gain:
   matches `wifi_scan.cpp:486-503` pattern's flicker reduction.

10. `wifi_deauth_extras.cpp:177-180` — Per-frame `fillScreen(0x0000)` +
    `ui_glitch` + scanline pass at 5Hz is the bulk of NUKE dashboard
    cost. Layer the glitch into a sprite that is `pushImage`d once per
    frame instead. Expected gain: lower CPU and lower TX-cache stall
    risk during 5G C5 delegation.

## Refactor targets

1. **Consolidate raw-IDF AP bring-up** — Three copies of the same
   ~50-line `esp_wifi_init → mode_ap → set_config → start → set_channel
   → settle` recipe exist (`wifi_portal.cpp:417-481`,
   `evil_twin.cpp:183-226`, `ap_signal_test.cpp:38-116`). Scope: extract
   to `wifi_ap_helpers.cpp:wifi_raw_ap_up(const char *ssid, uint8_t ch,
   bool open)`. Rationale: single point of truth, single point of fix
   when a Bruce-lib version changes the recipe. Risk: low — recipe is
   stable.

2. **Promisc cb teardown helper** — 8 features set
   `esp_wifi_set_promiscuous_rx_cb` but only `wifi_probe.cpp:465,513`
   clears it. Scope: extract
   `wifi_promisc_stop()` that clears cb + filter + promisc off.
   Rationale: defects 28+29. Risk: low.

3. **Split `wifi_portal.cpp`** (664 LOC) — Extract template HTML +
   picker UI into `wifi_portal_templates.cpp`; extract DNS+HTTP server
   loop into `wifi_portal_server.cpp`; keep entry + run_portal in main.
   Scope: 3 files at ~250 LOC each. Risk: low.

4. **Sanitize the dead `scan_task`** — `wifi_scan.cpp:56-177` is only
   reachable via Rescan key (`:531`). Inline scan at `:357-479` is the
   normal path. Unify the two paths; the duplication causes future
   bugs because fixes (e.g., WPS bit handling) have to land in two
   places. Risk: medium — task-mode + 8KB stack carries different
   failure modes.

5. **Drop Arduino `WiFi.softAP`/`mode(AP)` from `wifi_ciw.cpp`** —
   Single largest defect (CRIT 1+2+3). Scope: rewrite the AP mode
   bringup to use raw IDF + raw beacon TX. Reuses
   `wifi_beacon_spam.cpp`'s `build_beacon` pattern. Risk: medium —
   hardware-in-loop validation required (TX-stuck cold-boot first).

6. **Centralize PMF check** — `wifi_auth_has_pmf` is defined in
   `wifi_deauth_frame.h:238-253`, called in `wifi_deauth.cpp:221`,
   `evil_twin.cpp:384`. ApClone and Portal don't warn even though PMF
   makes deauth-driven re-association impossible. Scope: move into
   `wifi_ap_helpers` (refactor 1) and call from all kick-needing
   features.

7. **Make BTDM release optional / reversible** — 4 AP features
   unconditionally release BTDM, killing BLE for session. Scope: add
   global `g_btdm_released` flag + UI warning. If user has BLE
   features active, prompt "release BTDM? (BLE dies until reboot)
   [y/N]". Risk: low — pure UI gate.

8. **Move portal HTML to SD** — `wifi_portal_extras.h` (395 LOC) +
   `wifi_portal.cpp` inline templates (~280 LOC) reclaim ~30KB rodata
   if migrated to `/poseidon/portal/*.html`. Scope: add SD enumeration
   in pick_template, file streaming in handle_root. Risk: medium —
   SD I/O latency on serve, but acceptable (HTML one-shot per client).

9. **Per-feature promisc cb teardown audit** — combine refactor #2
   with a feature-template that exposes a `feature_enter`/`feature_exit`
   pair so common cleanup (cb=null, promisc=false, set channel=1,
   reset filter) cannot be skipped.

10. **Document `s_seq` ownership** — Three features (deauth,
    broadcast-deauth, hunt) keep their own `uint16_t s_seq`, but
    `wifi_deauth_pair` increments via `*seq` pointer. Make
    sequence ownership explicit by passing as struct, or use a global
    atomic.

## Backlog items

- **wifi-001** [CRIT] `wifi_ciw.cpp:238` — Replace `WiFi.mode(WIFI_MODE_AP)`/`WiFi.softAP` with raw-IDF AP recipe. Fix: lift `wifi_ap_helpers.cpp::wifi_raw_ap_up` (refactor 1) and call here. Effort: M.
- **wifi-002** [CRIT] `wifi_ciw.cpp:265` — Don't re-call `WiFi.softAP` per SSID rotation; mutate SSID via `esp_wifi_set_config(WIFI_IF_AP,...)` or rotate via raw beacon only. Fix: drop the softAP line, keep only `esp_wifi_80211_tx(WIFI_IF_AP, s_beacon, flen, false)`. Effort: S.
- **wifi-003** [HIGH] `wifi_ciw.cpp:296` — Replace `WiFi.mode(WIFI_MODE_APSTA)` teardown with `esp_wifi_stop(); esp_wifi_deinit();`. Fix: drop Arduino mode call, match raw-IDF teardown. Effort: S.
- **wifi-004** [HIGH] `wifi_portal.cpp:430` — Gate `esp_bt_controller_mem_release` behind a flag + UI warning. Fix: check `esp_bt_controller_get_status()`, only release if BLE never inited; emit toast warning. Effort: M.
- **wifi-005** [HIGH] `evil_twin.cpp:401` — Same gate as wifi-004 for evil_twin BTDM release. Fix: identical recipe. Effort: S.
- **wifi-006** [HIGH] `ap_signal_test.cpp:62` — Same gate as wifi-004 for sigtest BTDM release. Fix: identical recipe. Effort: S.
- **wifi-007** [HIGH] `wifi_portal.cpp:551` — Replace blocking 1.2s `ui_action_overlay` with non-blocking state-machine paint so DNS/HTTP don't stall. Fix: split overlay into per-frame ticks, render across multiple loop iters. Effort: M.
- **wifi-008** [HIGH] `evil_twin.cpp:337` — Same as wifi-007 inside evil_twin portal phase. Fix: same pattern. Effort: M.
- **wifi-009** [HIGH] `wifi_pmkid.cpp:447` — Convert sequential tone+delay notification to non-blocking state machine. Fix: schedule next tone via timer instead of `delay()`. Effort: M.
- **wifi-010** [HIGH] `wifi_scan.cpp:531` — Drop dead Rescan task path; collapse to inline scan reused via key handler. Fix: replace `xTaskCreate(scan_task,...)` with the inline scan logic from `:357-479`. Effort: M.
- **wifi-011** [HIGH] `wifi_ciw.cpp:222` — Move `active[PAYLOAD_COUNT]` from BSS to heap-allocated on entry; free on exit. Fix: `CiwPayload *active = (CiwPayload*)heap_caps_malloc(...)`. Effort: S.
- **wifi-012** [MED] `wifi_beacon_spam.cpp:40` — Make `s_beacon` a task-local stack buffer or lock with portMUX. Fix: move into `spam_task` and `build_beacon`. Effort: S.
- **wifi-013** [MED] `wifi_deauth_extras.cpp:34` — Unify silent_ap_begin scope with `wifi_clients_all.cpp` pattern. Fix: standardize on begin-once-per-feature. Effort: S.
- **wifi-014** [MED] `wifi_clients.cpp:90` — Async deauth burst from UI task. Fix: spawn brief task or cooperative tick across burst iterations. Effort: M.
- **wifi-015** [MED] `wifi_probe.cpp:222` — Move probe-response TX out of WiFi RX callback into a dedicated task + StreamBuffer. Fix: enqueue (sta+ssid+seq) from cb, TX from low-prio task. Effort: M.
- **wifi-016** [MED] `wifi_portal.cpp:497` — Convert `new DNSServer`/`new WebServer` to file-scope statics. Fix: lazy-init once, `begin()`/`stop()` to cycle. Effort: S.
- **wifi-017** [MED] `evil_twin.cpp:309` — Same as wifi-016. Fix: identical. Effort: S.
- **wifi-018** [MED] `wifi_pmkid.cpp:174` — Cap `m2_len` to 260 before `hex_append`. Fix: `if (m2_len > 260) m2_len = 260;` before line 175. Effort: S.
- **wifi-019** [MED] `wifi_pmkid.cpp:362` — `hunt_task` reads `s_cache[]` without snapshot. Fix: copy BSSID list under brief crit-section, then iterate snapshot. Effort: S.
- **wifi-020** [MED] `wifi_wardrive.cpp:93` — Per-AP `first_seen` field exists but timestamp written is the current `gps_snapshot.date`. Fix: store first-seen `g.date` at insert, use that in CSV row. Effort: S.
- **wifi-021** [MED] `wifi_wardrive.cpp:160` — GPS coords written even when `g.valid==false`. Fix: gate on `g.valid && a.first_gps_lat==0` to only set GPS once and never with zero coords. Effort: S.
- **wifi-022** [MED] `wifi_ciw.cpp:34` — Trim `ssid[64]` to `ssid[33]`; 802.11 max is 32. Fix: change struct field width + update `memcpy_P` use. Effort: S.
- **wifi-023** [MED] `wifi_deauth.cpp:44` — `s_seq` shared between resume cycles. Fix: read/write atomically or pass-by-value into task. Effort: S.
- **wifi-024** [MED] `wifi_clients_all.cpp:118` — `s_locked` toggle not atomic vs hop_task check. Fix: semaphore or atomic flag. Effort: S.
- **wifi-025** [MED] `wifi_portal.cpp:572` — Bump `delay(5)` to `delay(20)` when zero STAs connected. Fix: read `esp_wifi_ap_get_sta_list` once per loop, vary delay. Effort: S.
- **wifi-026** [LOW] `wifi_portal.cpp:434` — One-time `esp_netif_init`/`esp_event_loop_create_default`. Fix: gate with file-scope `static bool s_inited`. Effort: S.
- **wifi-027** [LOW] `wifi_beacon_spam.cpp:149` — Switch to `wifi_lean_sta_init()` from Arduino `WiFi.mode(WIFI_STA)`. Fix: 1-line swap. Effort: S.
- **wifi-028** [LOW] `wifi_spectrum.cpp:388` — Same as wifi-027. Fix: 1-line swap. Effort: S.
- **wifi-029** [LOW] `wifi_scan.cpp:67` — Remove dead retry path in `scan_task`. Fix: see wifi-010. Effort: S.
- **wifi-030** [LOW] `wifi_portal.cpp:166` — Migrate templates to SD `/poseidon/portal/`. Fix: refactor 8. Effort: L.
- **wifi-031** [LOW] `wifi_deauth_extras.cpp:55` — Don't fold C5 5GHz estimated frames into `s_b_sent`. Fix: separate counter + UI badge. Effort: S.
- **wifi-032** [LOW] `wifi_pmkid.cpp:554` — Clear `set_promiscuous_rx_cb(nullptr)` on exit. Fix: add 1 line before `s_out.close()`. Effort: S.
- **wifi-033** [LOW] All-features promisc cleanup audit. Fix: refactor 2 + apply across 8 sites. Effort: M.
- **wifi-034** [LOW] `wifi_portal.cpp` size — split into 3 files per refactor 3. Fix: split. Effort: M.
- **wifi-035** [HIGH] Verify `-Wl,-zmuldefs` is in POSEIDON's `platformio.ini` per memory invariant. Fix: confirm or add to `build_flags`. Effort: S.
- **wifi-036** [LOW] Add HE Capabilities IE to `wifi_beacon_spam.cpp` for Wi-Fi 6 spoofing (Ghost parity). Fix: append IE 0xFF (Extended Capabilities) per IEEE 802.11ax. Effort: M.
- **wifi-037** [LOW] Add AP-list beacon-spam mode (Ghost `-l` parity). Fix: pull SSIDs from `g_wdr_aps[]` and rotate. Effort: M.
- **wifi-038** [LOW] Add wardrive exclusion list (PORKCHOP BOAR BROS parity). Fix: fixed 50-entry array, persist to SD. Effort: M.
- **wifi-039** [LOW] Add EAPOL→PCAP output alongside hashcat 22000. Fix: per-frame PCAP write in `promisc_cb`. Effort: M.
- **wifi-040** [HIGH] Drop `STA→AP` reverse-pair frames in `wifi_deauth_pair` when broadcast (saves airtime, doubles effective per-burst rate). Fix: param flag `bool include_reverse` defaulting false for broadcast. Effort: S.
- **wifi-041** [MED] Apply PMF warning to ApClone + Portal entries (currently only Deauth + EvilTwin gate). Fix: call `wifi_auth_has_pmf` in `feat_wifi_apclone` and `feat_wifi_portal` clone paths. Effort: S.
- **wifi-042** [MED] Consolidate raw-IDF AP bring-up into `wifi_ap_helpers` (refactor 1). Fix: extract common code from 3 files. Effort: M.
- **wifi-043** [LOW] PORKCHOP reservation fence for heap layout. Fix: 80KB `heap_caps_malloc` + free pre-`wifi_lean_sta_init` in `radio.cpp::wifi_lean_sta_init`. Effort: M.
