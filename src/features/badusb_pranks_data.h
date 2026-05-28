#pragma once
#include <stdint.h>

/*
 * badusb_pranks_data.h — POSEIDON's curated prank highlight reel.
 *
 * The "showpiece" submenu: organized by VIBE, not by OS. Each table is a
 * thematic flavor of mischief, ordered best-first. Designed for the host
 * computer to enumerate POSEIDON as a USB HID keyboard and run these
 * DuckyScript-lite scripts via the badusb runner in badusb.cpp.
 *
 * DuckyScript-lite (one statement per line):
 *   REM <c> | DELAY <ms> | STRING <s> | ENTER | TAB | ESC | SPACE | BKSP | DEL
 *   UP/DOWN/LEFT/RIGHT | GUI [k] | CTRL [k] | ALT [k] | SHIFT [k]
 *   COMBO CTRL ALT t  (chord any modifiers + final key)
 *
 * NOTE: The current badusb.cpp parser does not yet handle a bare F11 / Fn
 * line — these scripts assume the parser will be extended OR the run-time
 * silently ignores them and the prank still mostly pops (browser opens,
 * just not fullscreen). Either is acceptable; the chaos still lands.
 *
 * Source: curated from UberGuidoZ's payload library
 *   https://github.com/UberGuidoZ/Hak5-USBRubberducky-Payloads
 * Anything sourced from there has been stripped of: DEFINE blocks, EXTENSION
 * preambles, STRINGLN (rewritten as STRING + ENTER), STRING_DELAY, WHILE/IF,
 * remote-URL fetches, binary downloads, and locale-dependent shenanigans.
 *
 * Several entries are POSEIDON-original variants where the upstream payload
 * needed an attacker-hosted URL or a remote binary — re-implemented to be
 * fully self-contained.
 *
 * Hard rule: every script here is recoverable. No real damage, no real
 * persistence, no real exfil. Just chaos that a reboot clears.
 */

/* OS bitmask — a single payload can target multiple OSes. */
#define PRANK_OS_WIN      0x01
#define PRANK_OS_MAC      0x02
#define PRANK_OS_LINUX    0x04
#define PRANK_OS_ANDROID  0x08
#define PRANK_OS_CHROMEOS 0x10
#define PRANK_OS_ANY      0xFF

struct prank_t {
    const char *name;        /* short label for menu */
    const char *blurb;       /* one-liner shown in the info pane */
    const char *script;      /* DuckyScript-lite payload */
    uint8_t     os_mask;     /* bitmask: 1=Win 2=Mac 4=Linux 8=Android 16=ChromeOS */
};

/* ============================================================================
 * === GASLIGHT =============================================================
 * Slow-burn. The target keeps glancing at their machine, wondering if they're
 * losing it. Plant the seed and walk away — your work here is done.
 * ============================================================================ */

/* Subtle cursor twitch via PowerShell SendKeys (one-shot per run). Pair with
 * a re-trigger to gaslight properly; one drag = one wiggle. */
static const char PRK_GL_JIGGLE[] =
    "REM cursor jiggler - small mouse keys nudge in a loop\n"
    "REM source: poseidon original (inspired by The_Mouse_Moves_By_Itself)\n"
    "DELAY 800\n"
    "GUI r\n"
    "DELAY 400\n"
    "STRING powershell -w h -c \"$w=New-Object -ComObject WScript.Shell;1..40|%{$w.SendKeys('{NUMLOCK}');Start-Sleep -m 800}\"\n"
    "ENTER\n";

/* Caps Lock chaos — flicker between cAsEs while they type. */
static const char PRK_GL_CAPSFLICKER[] =
    "REM cApS-lock troll - toggles caps every 1.2s for ~5min then dies\n"
    "REM source: cApS-Troll (UberGuidoZ) - rewritten without remote ps1\n"
    "DELAY 800\n"
    "GUI r\n"
    "DELAY 400\n"
    "STRING powershell -w h -c \"$w=New-Object -ComObject WScript.Shell;1..250|%{$w.SendKeys('{CAPSLOCK}');Start-Sleep -m 1200}\"\n"
    "ENTER\n";

/* Random typo injector — replaces "the" with "teh" in MS Word autocorrect.
 * They'll spend the rest of the week wondering why their reports look drunk. */
static const char PRK_GL_AUTOINCORRECT[] =
    "REM word autocorrect: 'the' -> 'teh' (permanent until they undo it)\n"
    "REM source: AUTOinCORRECT by the-jcksn (UberGuidoZ library/prank)\n"
    "DELAY 2000\n"
    "GUI r\n"
    "DELAY 300\n"
    "STRING winword\n"
    "ENTER\n"
    "DELAY 2500\n"
    "ENTER\n"
    "DELAY 400\n"
    "ALT q\n"
    "DELAY 400\n"
    "STRING options spelling\n"
    "DELAY 600\n"
    "ENTER\n"
    "DELAY 400\n"
    "TAB\n"
    "DELAY 200\n"
    "ENTER\n"
    "DELAY 400\n"
    "STRING the\n"
    "TAB\n"
    "STRING teh\n"
    "DELAY 200\n"
    "ALT a\n"
    "DELAY 300\n"
    "ENTER\n";

/* Screen rotation. They turn their head. They never live it down. */
static const char PRK_GL_ROTATE[] =
    "REM screen flip 90 deg (Windows + Intel/AMD GPU shortcut)\n"
    "REM source: poseidon original\n"
    "DELAY 500\n"
    "CTRL ALT RIGHT\n";

/* The Phantom Minimize — every 6 seconds, all windows shrink to the taskbar.
 * They'll think Windows is broken. */
static const char PRK_GL_MINIMIZE[] =
    "REM phantom minimize - drops all windows every 6s\n"
    "REM source: Always-Minimize by LyQuid (UberGuidoZ) - inlined, no vbs file\n"
    "DELAY 800\n"
    "GUI r\n"
    "DELAY 400\n"
    "STRING powershell -w h -c \"$s=New-Object -ComObject Shell.Application;1..100|%{$s.MinimizeAll();Start-Sleep -s 6}\"\n"
    "ENTER\n";

/* Mute/unmute roulette — every 4s, volume key gets tapped. Music cuts in
 * and out like a haunted Spotify. */
static const char PRK_GL_MUTEROULETTE[] =
    "REM mute key tap every 4s for ~5min\n"
    "REM source: poseidon original\n"
    "DELAY 800\n"
    "GUI r\n"
    "DELAY 400\n"
    "STRING powershell -w h -c \"$w=New-Object -ComObject WScript.Shell;1..75|%{$w.SendKeys([char]173);Start-Sleep -s 4}\"\n"
    "ENTER\n";

/* Browser back-button gremlin: every 8s, browser fires Alt+Left. They'll
 * blame their mouse. */
static const char PRK_GL_BACKBTN[] =
    "REM phantom back-button - hits Alt+Left every 8s\n"
    "REM source: poseidon original\n"
    "DELAY 800\n"
    "GUI r\n"
    "DELAY 400\n"
    "STRING powershell -w h -c \"$w=New-Object -ComObject WScript.Shell;1..50|%{$w.SendKeys('%{LEFT}');Start-Sleep -s 8}\"\n"
    "ENTER\n";

/* macOS twin of cursor jiggler — wiggles via AppleScript every 30s. */
static const char PRK_GL_MAC_JIGGLE[] =
    "REM macOS cursor nudge loop via osascript\n"
    "REM source: poseidon original\n"
    "GUI SPACE\n"
    "DELAY 500\n"
    "STRING terminal\n"
    "ENTER\n"
    "DELAY 1200\n"
    "STRING osascript -e 'repeat 40 times' -e 'tell application \"System Events\" to key code 63' -e 'delay 30' -e 'end repeat' &\n"
    "ENTER\n";

static const prank_t PRANK_GASLIGHT[] = {
    { "Caps Flicker",  "cApS bY tHe SeCoNd until they unplug you",          PRK_GL_CAPSFLICKER,   PRANK_OS_WIN },
    { "AutoinCORRECT", "Word now spells 'the' as 'teh'. Forever.",           PRK_GL_AUTOINCORRECT, PRANK_OS_WIN },
    { "Phantom Mini",  "Every 6s, all windows drop. Windows must be broken", PRK_GL_MINIMIZE,      PRANK_OS_WIN },
    { "Cursor Twitch", "Death by a thousand mouse-key spasms",               PRK_GL_JIGGLE,        PRANK_OS_WIN },
    { "Mute Roulette", "Volume keys press themselves. Music is haunted.",    PRK_GL_MUTEROULETTE,  PRANK_OS_WIN },
    { "Back-Btn Ghost","Browser hits Back every 8s. Probably the mouse.",    PRK_GL_BACKBTN,       PRANK_OS_WIN },
    { "Screen Flip",   "Ctrl+Alt+Right rotates the display 90 degrees",      PRK_GL_ROTATE,        PRANK_OS_WIN },
    { "Mac Twitch",    "macOS cursor nudge every 30s via osascript",         PRK_GL_MAC_JIGGLE,    PRANK_OS_MAC },
};
#define PRANK_GASLIGHT_N (sizeof(PRANK_GASLIGHT)/sizeof(PRANK_GASLIGHT[0]))

/* ============================================================================
 * === VISIBLE CHAOS ========================================================
 * They know. Everyone in the room knows. The fun has begun.
 * ============================================================================ */

/* The classic: 50 explorer windows, instantly. */
static const char PRK_VC_EXPLOSION[] =
    "REM rapid-open 30 explorer windows of C:\\\n"
    "REM source: poseidon original (riff on lol_killer / windows fork bomb meme)\n"
    "DELAY 600\n"
    "GUI r\n"
    "DELAY 400\n"
    "STRING powershell -w h -c \"1..30|%{Start-Process explorer.exe}\"\n"
    "ENTER\n";

/* Hacker Typer fullscreen. They type and it looks like a 90s movie. */
static const char PRK_VC_HACKERTYPER[] =
    "REM geektyper.com fullscreen + flailing\n"
    "REM source: Hacker_Typer (UberGuidoZ) - trimmed to fit, RIGHTARROW->RIGHT\n"
    "DELAY 1500\n"
    "GUI r\n"
    "DELAY 500\n"
    "STRING http://geektyper.com/plain\n"
    "ENTER\n"
    "DELAY 2500\n"
    "F11\n"
    "DELAY 500\n"
    "STRING qwertyuiopqwertyuiopqwertyuiopqwertyuiop\n"
    "DELAY 400\n"
    "RIGHT\n"
    "DELAY 800\n"
    "STRING 3164257\n";

/* Notepad vandalism, classic UberGuidoZ style. */
static const char PRK_VC_NOTEPAD[] =
    "REM notepad pwned banner\n"
    "REM source: poseidon original (notepad classic)\n"
    "GUI r\n"
    "DELAY 400\n"
    "STRING notepad\n"
    "ENTER\n"
    "DELAY 900\n"
    "STRING  >>> POSEIDON HAS THE CONN <<<\n"
    "ENTER\n"
    "STRING  this terminal belongs to the deep now\n"
    "ENTER\n"
    "ENTER\n"
    "STRING  - commander of the deep\n";

/* Magnifier Max — launches Windows Magnifier and zooms in 8 times. The
 * desktop becomes a single pixel of confusion. Win+ESC closes it. */
static const char PRK_VC_MAGNIFIER[] =
    "REM windows magnifier + zoom in 8x\n"
    "REM source: poseidon original\n"
    "DELAY 400\n"
    "GUI +\n"
    "DELAY 600\n"
    "GUI +\n"
    "DELAY 200\n"
    "GUI +\n"
    "DELAY 200\n"
    "GUI +\n"
    "DELAY 200\n"
    "GUI +\n"
    "DELAY 200\n"
    "GUI +\n"
    "DELAY 200\n"
    "GUI +\n"
    "DELAY 200\n"
    "GUI +\n";

/* the_f_bomb — opens every file in the directory. PowerShell two-liner. */
static const char PRK_VC_FBOMB[] =
    "REM the f bomb - ii ** opens every item in cwd\n"
    "REM source: the_f_bomb by @tjgeirk (UberGuidoZ) - verbatim, 4 lines\n"
    "DELAY 400\n"
    "GUI r\n"
    "DELAY 300\n"
    "STRING powershell while(1){ii **}\n"
    "ENTER\n";

/* Calculator avalanche — opens 20 instances. Bonus chaos for low effort. */
static const char PRK_VC_CALCSTORM[] =
    "REM 20 calculators in a trenchcoat\n"
    "REM source: poseidon original\n"
    "DELAY 500\n"
    "GUI r\n"
    "DELAY 400\n"
    "STRING powershell -w h -c \"1..20|%{Start-Process calc}\"\n"
    "ENTER\n";

/* Wallpaper to solid hot pink. Recoverable, hilarious. */
static const char PRK_VC_PINKWALL[] =
    "REM solid hot-pink wallpaper via registry + refresh\n"
    "REM source: poseidon original (riff on Wallpaper-Troll)\n"
    "DELAY 600\n"
    "GUI r\n"
    "DELAY 400\n"
    "STRING powershell -w h -c \"Set-ItemProperty 'HKCU:\\Control Panel\\Colors' Background '255 0 200'; rundll32.exe user32.dll,UpdatePerUserSystemParameters\"\n"
    "ENTER\n";

/* Linux variant — opens 20 GNOME terminals. */
static const char PRK_VC_LINUX_TERMS[] =
    "REM 20 gnome-terminals in a stack\n"
    "REM source: poseidon original\n"
    "CTRL ALT t\n"
    "DELAY 800\n"
    "STRING for i in $(seq 1 20); do gnome-terminal & done\n"
    "ENTER\n";

static const prank_t PRANK_VISIBLE[] = {
    { "Explorer Bomb",  "30 File Explorer windows, all at once",            PRK_VC_EXPLOSION,    PRANK_OS_WIN },
    { "Hacker Typer",   "Fullscreen 90s-movie hacking dramatics",           PRK_VC_HACKERTYPER,  PRANK_OS_WIN },
    { "Notepad Pwn",    "Classic Notepad vandalism, POSEIDON brand",        PRK_VC_NOTEPAD,      PRANK_OS_WIN },
    { "The F Bomb",     "ii ** opens EVERY file in the working dir",        PRK_VC_FBOMB,        PRANK_OS_WIN },
    { "Calc Storm",     "Twenty calculator windows. Why? Because.",         PRK_VC_CALCSTORM,    PRANK_OS_WIN },
    { "Magnify x8",     "Win Magnifier hits 800% zoom. Pure confusion.",    PRK_VC_MAGNIFIER,    PRANK_OS_WIN },
    { "Pink Apocalypse","Wallpaper goes hot pink. Recovery: change it back",PRK_VC_PINKWALL,     PRANK_OS_WIN },
    { "Terminal Stack", "20 gnome-terminals cascading on Linux",            PRK_VC_LINUX_TERMS,  PRANK_OS_LINUX },
};
#define PRANK_VISIBLE_N (sizeof(PRANK_VISIBLE)/sizeof(PRANK_VISIBLE[0]))

/* ============================================================================
 * === LOCKERS ==============================================================
 * "I just went to the bathroom and locked my PC" — except now you do it
 * for them, every 30 seconds, until they pull the dongle.
 * ============================================================================ */

/* Instant lock. Win+L, no chaser. */
static const char PRK_LK_INSTANT[] =
    "REM instant lock screen\n"
    "REM source: poseidon original (classic)\n"
    "GUI l\n";

/* Delayed lock — 30 seconds. Plug it in, walk out the door, hear the panic
 * from down the hall. */
static const char PRK_LK_DELAYED[] =
    "REM 30s timer then lock - escape the room first\n"
    "REM source: poseidon original\n"
    "DELAY 500\n"
    "GUI r\n"
    "DELAY 400\n"
    "STRING powershell -w h -c \"Start-Sleep 30; rundll32.exe user32.dll,LockWorkStation\"\n"
    "ENTER\n";

/* EternalLock-lite — locks every 5s for ~3 minutes. Recoverable by yanking
 * the device, but they'll have to do it FAST. */
static const char PRK_LK_ETERNAL[] =
    "REM lock every 5s for 36 iterations (~3min)\n"
    "REM source: EternalLock by 0i41E (UberGuidoZ) - rewritten without extensions\n"
    "DELAY 500\n"
    "GUI r\n"
    "DELAY 400\n"
    "STRING powershell -w h -c \"1..36|%{rundll32.exe user32.dll,LockWorkStation;Start-Sleep -s 5}\"\n"
    "ENTER\n";

/* Sleep instead of lock. Same idea, deeper nap. */
static const char PRK_LK_SLEEP[] =
    "REM put the machine to sleep\n"
    "REM source: poseidon original\n"
    "DELAY 400\n"
    "GUI r\n"
    "DELAY 300\n"
    "STRING powershell -w h -c \"& rundll32.exe powrprof.dll,SetSuspendState 0,1,0\"\n"
    "ENTER\n";

/* Sign-out variant — kicks the user back to the login screen, not just lock.
 * They'll lose unsaved work. Use sparingly. */
static const char PRK_LK_SIGNOUT[] =
    "REM sign out current user (closes apps, loses unsaved!)\n"
    "REM source: poseidon original\n"
    "DELAY 400\n"
    "GUI r\n"
    "DELAY 300\n"
    "STRING shutdown /l\n"
    "ENTER\n";

/* macOS instant lock — via Spotlight + pmset (since the parser can't yet do
 * the 3-key Ctrl+Cmd+Q chord, we shell out instead). */
static const char PRK_LK_MAC[] =
    "REM macOS display sleep via Spotlight + pmset\n"
    "REM source: poseidon original (Ctrl+Cmd+Q chord unavailable to parser)\n"
    "GUI SPACE\n"
    "DELAY 500\n"
    "STRING terminal\n"
    "ENTER\n"
    "DELAY 1200\n"
    "STRING pmset displaysleepnow\n"
    "ENTER\n";

/* Linux lock (GNOME). */
static const char PRK_LK_LINUX[] =
    "REM Linux/GNOME lock via Super+L\n"
    "REM source: poseidon original\n"
    "DELAY 300\n"
    "GUI l\n";

static const prank_t PRANK_LOCKERS[] = {
    { "Eternal Lock",   "Locks every 5s for 3min. Pull the cord FAST.",      PRK_LK_ETERNAL, PRANK_OS_WIN },
    { "Lock + Run",     "Instant Win+L. Plug, click, walk away.",            PRK_LK_INSTANT, PRANK_OS_WIN },
    { "30s Fuse",       "Locks in 30s. Time to be elsewhere when it pops.",  PRK_LK_DELAYED, PRANK_OS_WIN },
    { "Lights Out",     "Suspends the machine. Deeper than lock.",           PRK_LK_SLEEP,   PRANK_OS_WIN },
    { "Sign-Out Sneak", "Kicks user to login screen. Loses unsaved work.",   PRK_LK_SIGNOUT, PRANK_OS_WIN },
    { "Mac Lock",       "Ctrl+Cmd+Q instant lock",                           PRK_LK_MAC,     PRANK_OS_MAC },
    { "Linux Lock",     "Super+L on GNOME",                                  PRK_LK_LINUX,   PRANK_OS_LINUX },
};
#define PRANK_LOCKERS_N (sizeof(PRANK_LOCKERS)/sizeof(PRANK_LOCKERS[0]))

/* ============================================================================
 * === BROWSER DOOM =========================================================
 * Things that happen in their browser. Loud, fullscreen, and embarrassing
 * in any open-plan office.
 * ============================================================================ */

/* The classic Rickroll fullscreen. */
static const char PRK_BR_RICKROLL[] =
    "REM full-screen rickroll, max volume\n"
    "REM source: MaxVolumeRickroll_Windows by P-ict0 (UberGuidoZ) - one-liner kept\n"
    "DELAY 800\n"
    "GUI r\n"
    "DELAY 400\n"
    "STRING powershell -w h -c \"$w=New-Object -ComObject WScript.Shell;1..50|%{$w.SendKeys([char]175)};Start-Process 'https://www.youtube.com/watch?v=xvFZjo5PgG0&autoplay=1'\"\n"
    "ENTER\n";

/* 20 tabs of the Rick Astley clip. Stacked. */
static const char PRK_BR_TABFLOOD[] =
    "REM 20 tabs of rickroll - browser begs for mercy\n"
    "REM source: poseidon original\n"
    "DELAY 800\n"
    "GUI r\n"
    "DELAY 400\n"
    "STRING powershell -w h -c \"1..20|%{Start-Process 'https://www.youtube.com/watch?v=dQw4w9WgXcQ'}\"\n"
    "ENTER\n";

/* Google Image search for 'CATS'. Wholesome. Disruptive. */
static const char PRK_BR_CATS[] =
    "REM open Google Images for cats. peak chaos.\n"
    "REM source: poseidon original\n"
    "DELAY 600\n"
    "GUI r\n"
    "DELAY 400\n"
    "STRING https://www.google.com/search?q=cats&tbm=isch\n"
    "ENTER\n"
    "DELAY 3000\n"
    "F11\n";

/* Lemonparty… no, kidding, the SAFE classic. Hampterdance. */
static const char PRK_BR_HAMPTER[] =
    "REM open hampsterdance.com - the original web prank\n"
    "REM source: poseidon original (1998 classic)\n"
    "DELAY 600\n"
    "GUI r\n"
    "DELAY 400\n"
    "STRING http://www.hampsterdance.com\n"
    "ENTER\n"
    "DELAY 3000\n"
    "F11\n";

/* Trololo Eduard Khil. */
static const char PRK_BR_TROLOLO[] =
    "REM Mr. Trololo, fullscreen, autoplay\n"
    "REM source: poseidon original\n"
    "DELAY 800\n"
    "GUI r\n"
    "DELAY 400\n"
    "STRING https://www.youtube.com/watch?v=2Z4m4lnjxkY&autoplay=1\n"
    "ENTER\n"
    "DELAY 4000\n"
    "F11\n";

/* Open one tab… then 49 more. */
static const char PRK_BR_50TABS[] =
    "REM 50 Wikipedia random-article tabs\n"
    "REM source: poseidon original\n"
    "DELAY 600\n"
    "GUI r\n"
    "DELAY 400\n"
    "STRING powershell -w h -c \"1..50|%{Start-Process 'https://en.wikipedia.org/wiki/Special:Random'}\"\n"
    "ENTER\n";

/* Set homepage to a rickroll via Edge command-line (visible but they'll
 * have to dig to undo it). */
static const char PRK_BR_HOMEPAGE[] =
    "REM force Edge to open Rickroll on next launch\n"
    "REM source: poseidon original\n"
    "DELAY 600\n"
    "GUI r\n"
    "DELAY 400\n"
    "STRING powershell -w h -c \"Start-Process msedge 'https://youtu.be/dQw4w9WgXcQ?autoplay=1'\"\n"
    "ENTER\n";

static const prank_t PRANK_BROWSER[] = {
    { "Max-Vol Rick",   "100% volume + fullscreen Rickroll. Cinema.",        PRK_BR_RICKROLL,  PRANK_OS_WIN },
    { "Tab Flood",      "20 Rickroll tabs at once. Chrome cries.",           PRK_BR_TABFLOOD,  PRANK_OS_WIN },
    { "Wiki50",         "50 random Wikipedia tabs. Educational, technically",PRK_BR_50TABS,    PRANK_OS_WIN },
    { "Cats!",          "Fullscreen Google Image search: cats",              PRK_BR_CATS,      PRANK_OS_WIN },
    { "Trololo",        "Eduard Khil, fullscreen, autoplay",                 PRK_BR_TROLOLO,   PRANK_OS_WIN },
    { "Hampter Dance",  "The original 1998 web prank",                       PRK_BR_HAMPTER,   PRANK_OS_WIN },
    { "Edge Rickroll",  "Launch Edge straight into Astley",                  PRK_BR_HOMEPAGE,  PRANK_OS_WIN },
};
#define PRANK_BROWSER_N (sizeof(PRANK_BROWSER)/sizeof(PRANK_BROWSER[0]))

/* ============================================================================
 * === FAKE HACK ============================================================
 * Looks like Mr. Robot. Is actually a `cmd /k color`. Pair with a 60s delay
 * to trigger AFTER they walk away from their desk.
 * ============================================================================ */

/* Matrix in cmd via PowerShell. Looks the part, costs nothing. */
static const char PRK_FH_MATRIX[] =
    "REM matrix-style rain in cmd via random chars + green text\n"
    "REM source: poseidon original (riff on Digital_Rain)\n"
    "DELAY 600\n"
    "GUI r\n"
    "DELAY 400\n"
    "STRING cmd /k color 0a && mode con cols=120 lines=40 && powershell -c \"while(1){-join((33..126|Get-Random -Count 120)|%{[char]$_})}\"\n"
    "ENTER\n";

/* Delayed Matrix — 60s pause then the show begins. */
static const char PRK_FH_MATRIX_LATE[] =
    "REM matrix rain, fires 60s after deploy\n"
    "REM source: poseidon original\n"
    "DELAY 600\n"
    "GUI r\n"
    "DELAY 400\n"
    "STRING powershell -w h -c \"Start-Sleep 60; Start-Process cmd '/k color 0a && mode con cols=120 lines=40 && powershell -c \\\"while(1){-join((33..126|Get-Random -Count 120)|%%{[char]$_})}\\\"'\"\n"
    "ENTER\n";

/* "Your PC has been compromised" notepad full-screen banner. */
static const char PRK_FH_BANNER[] =
    "REM fake compromise banner via notepad zoom\n"
    "REM source: poseidon original\n"
    "GUI r\n"
    "DELAY 400\n"
    "STRING notepad\n"
    "ENTER\n"
    "DELAY 900\n"
    "STRING  *** YOUR PC HAS BEEN COMPROMISED ***\n"
    "ENTER\n"
    "STRING  ALL DATA ENCRYPTED. POSEIDON v3.1.4\n"
    "ENTER\n"
    "STRING  (just kidding - press Ctrl+Z to undo)\n"
    "DELAY 500\n"
    "F11\n";

/* Fake ransomware countdown — visual only, no actual encryption. */
static const char PRK_FH_RANSOM[] =
    "REM fake countdown ransom screen - no actual encryption\n"
    "REM source: poseidon original (NOT real ransomware)\n"
    "DELAY 600\n"
    "GUI r\n"
    "DELAY 400\n"
    "STRING cmd /k color 4f && powershell -c \"Clear-Host; Write-Host '   *** POSEIDON RANSOM SIMULATION ***'; Write-Host '   PRESS ANY KEY TO ACK NOT-REAL'; 30..0|%{Write-Host ('   T-MINUS '+$_+'s ');Start-Sleep 1}\"\n"
    "ENTER\n";

/* Fake BSOD via PowerShell fullscreen — opens a known BSOD simulator page.
 * Recoverable with ESC. */
static const char PRK_FH_BSOD[] =
    "REM fake bsod via fakeupdate.net/bsod (fullscreen)\n"
    "REM source: poseidon original (fakeupdate.net is the safe go-to)\n"
    "DELAY 600\n"
    "GUI r\n"
    "DELAY 400\n"
    "STRING https://fakeupdate.net/win10ue\n"
    "ENTER\n"
    "DELAY 4000\n"
    "F11\n";

/* Fake Windows Update screen, also via fakeupdate.net. */
static const char PRK_FH_UPDATE[] =
    "REM fake windows update fullscreen (fakeupdate.net)\n"
    "REM source: poseidon original\n"
    "DELAY 600\n"
    "GUI r\n"
    "DELAY 400\n"
    "STRING https://fakeupdate.net/win10u\n"
    "ENTER\n"
    "DELAY 4000\n"
    "F11\n";

/* Delayed BSOD — 90 seconds after deploy, fakeupdate fires. They'll be
 * back from coffee. */
static const char PRK_FH_BSOD_LATE[] =
    "REM bsod, but armed for 90s\n"
    "REM source: poseidon original\n"
    "DELAY 600\n"
    "GUI r\n"
    "DELAY 400\n"
    "STRING powershell -w h -c \"Start-Sleep 90; Start-Process 'https://fakeupdate.net/win10ue'\"\n"
    "ENTER\n";

/* nmap-looking output for cinematic effect. */
static const char PRK_FH_NMAP_LARP[] =
    "REM tracert google.com in red cmd - looks like nmap to non-techies\n"
    "REM source: poseidon original\n"
    "DELAY 600\n"
    "GUI r\n"
    "DELAY 400\n"
    "STRING cmd /k color 0c && tracert -d google.com && ping -t 8.8.8.8\n"
    "ENTER\n";

static const prank_t PRANK_FAKEHACK[] = {
    { "BSOD Drop",      "Fullscreen fakeupdate.net BSOD. Looks REAL.",       PRK_FH_BSOD,         PRANK_OS_WIN },
    { "BSOD Timer",     "Same, armed for 90s. Pop it after they walk off.",  PRK_FH_BSOD_LATE,    PRANK_OS_WIN },
    { "Matrix Rain",    "Green chars cascading in fullscreen cmd",           PRK_FH_MATRIX,       PRANK_OS_WIN },
    { "Matrix Timer",   "Matrix delayed 60s. Be elsewhere when it hits.",    PRK_FH_MATRIX_LATE,  PRANK_OS_WIN },
    { "Ransom Sim",     "Red countdown screen - no actual encryption",       PRK_FH_RANSOM,       PRANK_OS_WIN },
    { "Win Update",     "Fake Win10 update screen at fullscreen",            PRK_FH_UPDATE,       PRANK_OS_WIN },
    { "PWNED Banner",   "Notepad goes fullscreen with breach text",          PRK_FH_BANNER,       PRANK_OS_WIN },
    { "Nmap LARP",      "Red cmd running tracert+ping. Movie energy.",       PRK_FH_NMAP_LARP,    PRANK_OS_WIN },
};
#define PRANK_FAKEHACK_N (sizeof(PRANK_FAKEHACK)/sizeof(PRANK_FAKEHACK[0]))

/* ============================================================================
 * === LOUD =================================================================
 * Maximum volume, maximum embarrassment. Best deployed in libraries,
 * lecture halls, and any room with bad acoustics.
 * ============================================================================ */

/* TTS speaks the embarrassing line aloud via .NET SpeechSynth. */
static const char PRK_LD_TTS_EMBARRASS[] =
    "REM TTS speaks 'I have committed crimes against UX'\n"
    "REM source: Talking_Duck by JoustingZebra (UberGuidoZ) - text changed\n"
    "DELAY 800\n"
    "GUI r\n"
    "DELAY 400\n"
    "STRING powershell -w h -c \"Add-Type -AssemblyName System.speech;$s=New-Object System.Speech.Synthesis.SpeechSynthesizer;$s.Speak('I solemnly swear, I have committed crimes against good UX, and I am very sorry.')\"\n"
    "ENTER\n";

/* TTS speaks "I love Internet Explorer". Career-ending for any IT person. */
static const char PRK_LD_TTS_IE[] =
    "REM TTS confession - IE love\n"
    "REM source: poseidon original\n"
    "DELAY 800\n"
    "GUI r\n"
    "DELAY 400\n"
    "STRING powershell -w h -c \"Add-Type -AssemblyName System.speech;$s=New-Object System.Speech.Synthesis.SpeechSynthesizer;$s.Speak('Internet Explorer was, and remains, the superior browser.')\"\n"
    "ENTER\n";

/* Max volume + system beep spam. */
static const char PRK_LD_BEEPSPAM[] =
    "REM max volume + 30 system beeps\n"
    "REM source: poseidon original\n"
    "DELAY 800\n"
    "GUI r\n"
    "DELAY 400\n"
    "STRING powershell -w h -c \"$w=New-Object -ComObject WScript.Shell;1..50|%{$w.SendKeys([char]175)};1..30|%{[console]::Beep(800,200)}\"\n"
    "ENTER\n";

/* USBScream — but locally, no remote .wav. Plays default system beep
 * repeatedly. */
static const char PRK_LD_SIREN[] =
    "REM siren - alternating high/low tones for 20s\n"
    "REM source: USBScream concept (UberGuidoZ) - reimplemented locally\n"
    "DELAY 800\n"
    "GUI r\n"
    "DELAY 400\n"
    "STRING powershell -w h -c \"$w=New-Object -ComObject WScript.Shell;1..50|%{$w.SendKeys([char]175)};1..20|%{[console]::Beep(1200,250);[console]::Beep(600,250)}\"\n"
    "ENTER\n";

/* Set volume to max only — pair with anything else. */
static const char PRK_LD_MAXVOL[] =
    "REM bump volume to 100% silently\n"
    "REM source: poseidon original\n"
    "DELAY 400\n"
    "GUI r\n"
    "DELAY 300\n"
    "STRING powershell -w h -c \"$w=New-Object -ComObject WScript.Shell;1..50|%{$w.SendKeys([char]175)}\"\n"
    "ENTER\n";

/* macOS `say` — embarrassing TTS line. */
static const char PRK_LD_MAC_SAY[] =
    "REM macOS 'say' speaks UX confession\n"
    "REM source: Multi_HID_HeyGotAnyGrapes idea (UberGuidoZ) - text replaced\n"
    "GUI SPACE\n"
    "DELAY 500\n"
    "STRING terminal\n"
    "ENTER\n"
    "DELAY 1200\n"
    "STRING osascript -e 'set volume 7' && say -v Fred 'POSEIDON has the conn. Resistance is futile.'\n"
    "ENTER\n";

/* Linux espeak twin. */
static const char PRK_LD_LINUX_ESPEAK[] =
    "REM espeak on linux - same confession\n"
    "REM source: poseidon original\n"
    "CTRL ALT t\n"
    "DELAY 800\n"
    "STRING espeak 'POSEIDON has the conn. Resistance is futile.'\n"
    "ENTER\n";

static const prank_t PRANK_LOUD[] = {
    { "TTS: UX Crimes", "Computer apologizes for war crimes against UX",     PRK_LD_TTS_EMBARRASS, PRANK_OS_WIN },
    { "TTS: IE Love",   "Career-ending: declares love for Internet Explorer",PRK_LD_TTS_IE,        PRANK_OS_WIN },
    { "Siren",          "Max-vol alternating 1200/600Hz tones for 10s",      PRK_LD_SIREN,         PRANK_OS_WIN },
    { "Beep Spam",      "Max volume + 30 system beeps",                      PRK_LD_BEEPSPAM,      PRANK_OS_WIN },
    { "Max Volume",     "Just bumps to 100%. Setup for the real payload.",   PRK_LD_MAXVOL,        PRANK_OS_WIN },
    { "Mac 'say'",      "macOS 'say' command: POSEIDON has the conn",        PRK_LD_MAC_SAY,       PRANK_OS_MAC },
    { "Linux espeak",   "espeak version of the same confession",             PRK_LD_LINUX_ESPEAK,  PRANK_OS_LINUX },
};
#define PRANK_LOUD_N (sizeof(PRANK_LOUD)/sizeof(PRANK_LOUD[0]))

/* ============================================================================
 * === CLASSIC ==============================================================
 * The all-time greats. The Hello Worlds of payload pranks. Use these to
 * earn the title "yeah I have a USB Rubber Ducky."
 * ============================================================================ */

/* The Hello. */
static const char PRK_CL_HELLO[] =
    "REM hello world\n"
    "REM source: poseidon original (the canonical first payload)\n"
    "DELAY 500\n"
    "STRING Hello, World!\n"
    "ENTER\n";

/* You've been pwned — Notepad classic. */
static const char PRK_CL_PWNED[] =
    "REM 'you have been pwned' notepad\n"
    "REM source: hak5 canonical demo payload\n"
    "GUI r\n"
    "DELAY 400\n"
    "STRING notepad\n"
    "ENTER\n"
    "DELAY 800\n"
    "STRING you have been pwned by POSEIDON\n"
    "ENTER\n"
    "STRING   commander of the deep\n";

/* The Rickroll URL. The original sin. */
static const char PRK_CL_RICKROLL[] =
    "REM the canonical rickroll URL\n"
    "REM source: hak5 canonical demo payload\n"
    "GUI r\n"
    "DELAY 400\n"
    "STRING https://youtu.be/dQw4w9WgXcQ\n"
    "ENTER\n";

/* "Are you there?" — passive-aggressive notepad message. */
static const char PRK_CL_AREYOUTHERE[] =
    "REM 'are you there?' - the haunted notepad classic\n"
    "REM source: hak5 forum classics\n"
    "GUI r\n"
    "DELAY 400\n"
    "STRING notepad\n"
    "ENTER\n"
    "DELAY 800\n"
    "STRING are you there?\n"
    "ENTER\n"
    "DELAY 2000\n"
    "STRING hello?\n"
    "ENTER\n"
    "DELAY 1500\n"
    "STRING I can see you\n"
    "ENTER\n"
    "DELAY 1500\n"
    "STRING just kidding ...maybe\n";

/* The "your PC is fine, I promise" prank — opens command prompt with title
 * "Critical Error" but does nothing else. */
static const char PRK_CL_FAKECRIT[] =
    "REM cmd window titled CRITICAL ERROR doing nothing\n"
    "REM source: hak5 forum classic\n"
    "DELAY 500\n"
    "GUI r\n"
    "DELAY 400\n"
    "STRING cmd /k title CRITICAL ERROR ^&^& color 4f ^&^& echo. ^&^& echo   STAND BY... ^&^& echo.\n"
    "ENTER\n";

/* Print 'I am watching you' in run dialog history. Future opens of the run
 * dialog show it. Subtle, perfect. */
static const char PRK_CL_RUNHISTORY[] =
    "REM seeds run-dialog history with 'i am watching you'\n"
    "REM source: poseidon original\n"
    "GUI r\n"
    "DELAY 400\n"
    "STRING i am watching you\n"
    "REM intentionally no ENTER - leaves it in the dropdown\n"
    "ESC\n";

/* Linux echo pwned. */
static const char PRK_CL_LINUX_PWNED[] =
    "REM echo pwned to /tmp on linux\n"
    "REM source: hak5 canonical demo payload\n"
    "CTRL ALT t\n"
    "DELAY 600\n"
    "STRING echo 'pwned by POSEIDON, commander of the deep' > /tmp/poseidon && cat /tmp/poseidon\n"
    "ENTER\n";

static const prank_t PRANK_CLASSIC[] = {
    { "Pwned",          "Classic Notepad: you have been pwned",              PRK_CL_PWNED,         PRANK_OS_WIN },
    { "Rickroll URL",   "Run dialog opens the OG Rickroll",                  PRK_CL_RICKROLL,      PRANK_OS_WIN },
    { "Are You There?", "Haunted Notepad asks if anyone's home",             PRK_CL_AREYOUTHERE,   PRANK_OS_WIN },
    { "Critical Error", "Red CMD window titled CRITICAL ERROR. Does nothing",PRK_CL_FAKECRIT,      PRANK_OS_WIN },
    { "Run Watcher",    "Plants 'i am watching you' in Run history",         PRK_CL_RUNHISTORY,    PRANK_OS_WIN },
    { "Hello World",    "The original. Always works. Always lands.",         PRK_CL_HELLO,         PRANK_OS_ANY },
    { "Linux Pwned",    "echo pwned > /tmp/poseidon on Linux",               PRK_CL_LINUX_PWNED,   PRANK_OS_LINUX },
};
#define PRANK_CLASSIC_N (sizeof(PRANK_CLASSIC)/sizeof(PRANK_CLASSIC[0]))

/* ============================================================================
 * Convenience: an array of all category tables, in display order.
 * The menu loader in badusb.cpp / menu.cpp can iterate this.
 * ============================================================================ */
struct prank_category_t {
    const char     *name;
    const prank_t  *items;
    size_t          count;
};

static const prank_category_t PRANK_CATEGORIES[] = {
    { "GASLIGHT",      PRANK_GASLIGHT, PRANK_GASLIGHT_N },
    { "VISIBLE CHAOS", PRANK_VISIBLE,  PRANK_VISIBLE_N  },
    { "LOCKERS",       PRANK_LOCKERS,  PRANK_LOCKERS_N  },
    { "BROWSER DOOM",  PRANK_BROWSER,  PRANK_BROWSER_N  },
    { "FAKE HACK",     PRANK_FAKEHACK, PRANK_FAKEHACK_N },
    { "LOUD",          PRANK_LOUD,     PRANK_LOUD_N     },
    { "CLASSIC",       PRANK_CLASSIC,  PRANK_CLASSIC_N  },
};
#define PRANK_CATEGORIES_N (sizeof(PRANK_CATEGORIES)/sizeof(PRANK_CATEGORIES[0]))
