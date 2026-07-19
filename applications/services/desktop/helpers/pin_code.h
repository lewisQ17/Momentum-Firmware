#pragma once

#include <stdint.h>
#include <stdbool.h>

#define DESKTOP_PIN_CODE_MIN_LEN (4)
#define DESKTOP_PIN_CODE_MAX_LEN (10)

#define DESKTOP_SETTINGS_RUN_PIN_SETUP_ARG "run_pin_setup"

typedef struct {
    uint8_t data[DESKTOP_PIN_CODE_MAX_LEN];
    uint8_t length;
} DesktopPinCode;

bool desktop_pin_code_is_set(void);

void desktop_pin_code_set(const DesktopPinCode* pin_code);

void desktop_pin_code_reset(void);

bool desktop_pin_code_check(const DesktopPinCode* pin_code);

bool desktop_pin_code_is_equal(const DesktopPinCode* pin_code1, const DesktopPinCode* pin_code2);

/** Optional "duress" PIN — an opt-in second PIN that, when entered at the
 * lockscreen, silently wipes the SD card + RTC state and reboots instead of
 * unlocking. Stored in its own RTC backup register (never on the SD card).
 * All functions are no-ops / false when no duress PIN is configured. */

/** @return true if a duress PIN is currently configured */
bool desktop_duress_pin_is_set(void);

/** Store the duress PIN (packed into its RTC register). Caller MUST ensure it
 * differs from the unlock PIN. */
void desktop_pin_code_set_duress(const DesktopPinCode* pin_code);

/** Clear any configured duress PIN */
void desktop_pin_code_reset_duress(void);

/** @return true only if a duress PIN is set AND the given code matches it */
bool desktop_pin_code_is_duress(const DesktopPinCode* pin_code);

void desktop_pin_lock_error_notify(void);

uint32_t desktop_pin_lock_get_fail_timeout(void);
