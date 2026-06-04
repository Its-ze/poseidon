/*
 * ir_clone.h — public surface for the IR clone / blast utilities.
 *
 * Previously these were forward-declared inline inside
 * ir_extras_data.h (file scope) which leaked the declarations
 * into every TU that included the prank-data header — and one of
 * those decls was `extern void delay(uint32_t)`, conflicting with
 * Arduino.h's `void delay(unsigned long)` if both headers reached
 * the same TU. Phase 0 hygiene (ir-003) routes everything through
 * a proper header.
 */
#pragma once

#include <stdint.h>

/* Bit-banged blast on the IR LED. carrier_khz typically 38; pairs is
 * a NULL-terminated alternating on/off duration table (units = µs,
 * sentinel = 0,0). Defined in ir_clone.cpp. */
void blast_raw(uint16_t carrier_khz, const uint16_t *pairs);

/* Brand-specific helpers used by the prank tables. */
void send_samsung(uint8_t cmd);
void send_lg(uint8_t cmd);
