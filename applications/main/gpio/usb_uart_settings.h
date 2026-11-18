#pragma once

#include "usb_uart_bridge.h"

#include <stdbool.h>
#include <stdint.h>

/** Fixed baudrates offered by the config menu (index 0 of the menu is "Host") */
#define USB_UART_BAUDRATE_COUNT (12)
extern const uint32_t usb_uart_baudrate_list[USB_UART_BAUDRATE_COUNT];

/* Option counts of the config menu, used to range-check stored values */
#define USB_UART_VCP_CH_COUNT           (2) /**< 0 (CLI), 1 */
#define USB_UART_FLOW_PINS_COUNT        (4) /**< None, "2,3", "6,7", "16,15" */
#define USB_UART_FLOW_PINS_LPUART_COUNT (3) /**< 16,15 are taken by the LPUART itself */
#define USB_UART_DE_RE_COUNT            (2) /**< None, 4 */

/** Load the saved bridge config
 *
 * Without a (valid) settings file the config is the historical default: VCP 0,
 * pins 13/14, no flow pins, host baudrate, no DE/RE, 8N1 from the menu and no
 * RTS/DTR inversion. Every stored value is range-checked on its own and falls
 * back to its default when it is not something the config menu can produce.
 *
 * @param[out] cfg   config to fill, always left in a valid state
 *
 * @return     true if a settings file was loaded
 */
bool usb_uart_settings_load(UsbUartConfig* cfg);

/** Save the bridge config
 *
 * Does not touch the storage when the config equals what is already stored
 * (or equals the defaults while no settings file exists).
 *
 * @param[in]  cfg   config to save
 *
 * @return     true if the stored settings match cfg afterwards
 */
bool usb_uart_settings_save(const UsbUartConfig* cfg);
