#pragma once

#include <stdint.h>
#include <stdbool.h>

typedef struct UsbUartBridge UsbUartBridge;

/** Where the UART framing (data/parity/stop bits) comes from */
typedef enum {
    UsbUartFramingSrcMenu, /**< Fixed framing from the config menu (default 8N1) */
    UsbUartFramingSrcHost, /**< Framing requested by the host (CDC SET_LINE_CODING) */

    UsbUartFramingSrcCount,
} UsbUartFramingSrc;

/* Zero value of every framing enum is the historical default (8N1) */
typedef enum {
    UsbUartDataBits8,
    UsbUartDataBits7,
    UsbUartDataBits6, /**< Only valid together with parity */

    UsbUartDataBitsCount,
} UsbUartDataBits;

typedef enum {
    UsbUartParityNone,
    UsbUartParityEven,
    UsbUartParityOdd,

    UsbUartParityCount,
} UsbUartParity;

typedef enum {
    UsbUartStopBits1,
    UsbUartStopBits2,
    UsbUartStopBits1_5, /**< USART only */
    UsbUartStopBits0_5, /**< USART only */

    UsbUartStopBitsCount,
} UsbUartStopBits;

/** Number of stop bit options supported by LPUART (whole bit lengths only) */
#define USB_UART_LPUART_STOP_BITS_COUNT (UsbUartStopBits2 + 1)

/** RTS/DTR output inversion, bitmask. 0 keeps the classic active-low lines */
typedef enum {
    UsbUartFlowInvertNone = 0,
    UsbUartFlowInvertRts = (1 << 0),
    UsbUartFlowInvertDtr = (1 << 1),
    UsbUartFlowInvertBoth = UsbUartFlowInvertRts | UsbUartFlowInvertDtr,
} UsbUartFlowInvert;

typedef struct {
    uint8_t vcp_ch;
    uint8_t uart_ch;
    uint8_t flow_pins;
    uint8_t baudrate_mode;
    uint32_t baudrate;
    uint8_t software_de_re;
    uint8_t framing_src; // UsbUartFramingSrc
    uint8_t data_bits; // UsbUartDataBits, used when framing_src == Menu
    uint8_t parity; // UsbUartParity, used when framing_src == Menu
    uint8_t stop_bits; // UsbUartStopBits, used when framing_src == Menu
    uint8_t flow_invert; // UsbUartFlowInvert bitmask
} UsbUartConfig;

typedef struct {
    uint32_t rx_cnt;
    uint32_t tx_cnt;
    uint32_t baudrate_cur;
    uint8_t data_bits_cur; // UsbUartDataBits currently applied
    uint8_t parity_cur; // UsbUartParity currently applied
    uint8_t stop_bits_cur; // UsbUartStopBits currently applied
} UsbUartState;

/** Check whether a framing combination is supported on the given UART
 *
 * @param      uart_ch    FuriHalSerialId of the UART
 * @param      data_bits  UsbUartDataBits
 * @param      parity     UsbUartParity
 * @param      stop_bits  UsbUartStopBits
 *
 * @return     true if the hardware supports it
 */
bool usb_uart_framing_is_valid(
    uint8_t uart_ch,
    uint8_t data_bits,
    uint8_t parity,
    uint8_t stop_bits);

UsbUartBridge* usb_uart_enable(UsbUartConfig* cfg);

void usb_uart_disable(UsbUartBridge* usb_uart);

void usb_uart_set_config(UsbUartBridge* usb_uart, UsbUartConfig* cfg);

void usb_uart_get_config(UsbUartBridge* usb_uart, UsbUartConfig* cfg);

void usb_uart_get_state(UsbUartBridge* usb_uart, UsbUartState* st);
