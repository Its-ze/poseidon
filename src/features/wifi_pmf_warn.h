/*
 * wifi_pmf_warn — shared "PMF will block this attack" warning UI.
 *
 * POS-AUDIT-213 / wifi-041: wifi_deauth.cpp checks wifi_auth_has_pmf
 * before firing and surfaces a confirmation overlay; wifi_portal +
 * wifi_apclone clone paths also benefit because PMF blocks the
 * deauth-driven re-association that drives the lure. Promote the
 * helper from a static in wifi_deauth.cpp so the clone paths can
 * reuse it instead of silently proceeding against PMF targets.
 *
 * Returns true if the user pressed ENTER to proceed anyway, false on
 * ESC (back out of the feature).
 */
#pragma once

bool wifi_pmf_warning(void);
