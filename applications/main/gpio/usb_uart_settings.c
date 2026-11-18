#include "usb_uart_settings.h"

#include <furi.h>
#include <furi_hal_serial_types.h>
#include <storage/storage.h>
#include <toolbox/saved_struct.h>

#define USB_UART_SETTINGS_PATH    INT_PATH(".gpio_usb_uart.settings")
#define USB_UART_SETTINGS_MAGIC   (0x55)
#define USB_UART_SETTINGS_VERSION (1)

const uint32_t usb_uart_baudrate_list[USB_UART_BAUDRATE_COUNT] = {
    1200,
    2400,
    4800,
    9600,
    19200,
    28800,
    38400,
    57600,
    115200,
    230400,
    460800,
    921600,
};

// On-disk layout. Independent of UsbUartConfig so that struct can change freely;
// changing this one needs a version bump.
typedef struct {
    uint32_t baudrate; // 0 = follow the host
    uint8_t vcp_ch;
    uint8_t uart_ch;
    uint8_t flow_pins;
    uint8_t software_de_re;
    uint8_t framing_src;
    uint8_t data_bits;
    uint8_t parity;
    uint8_t stop_bits;
    uint8_t flow_invert;
    uint8_t reserved[3];
} UsbUartSettingsFile;

_Static_assert(sizeof(UsbUartSettingsFile) == 16, "layout changed: bump the settings version");

// Replace everything the config menu could not have produced by its default.
// The defaults are all zero, exactly what the bridge used before it had settings.
static void usb_uart_settings_sanitize(UsbUartSettingsFile* file) {
    if(file->vcp_ch >= USB_UART_VCP_CH_COUNT) file->vcp_ch = 0;

    if(file->uart_ch != FuriHalSerialIdUsart && file->uart_ch != FuriHalSerialIdLpuart) {
        file->uart_ch = FuriHalSerialIdUsart;
    }

    // Pins 16,15 cannot carry RTS/DTR while the LPUART is using them
    const uint8_t flow_pins_count = (file->uart_ch == FuriHalSerialIdLpuart) ?
                                        USB_UART_FLOW_PINS_LPUART_COUNT :
                                        USB_UART_FLOW_PINS_COUNT;
    if(file->flow_pins >= flow_pins_count) file->flow_pins = 0;

    if(file->software_de_re >= USB_UART_DE_RE_COUNT) file->software_de_re = 0;

    // Only baudrates from the menu are accepted, anything else follows the host
    bool baudrate_known = false;
    for(size_t i = 0; i < USB_UART_BAUDRATE_COUNT; i++) {
        if(file->baudrate == usb_uart_baudrate_list[i]) {
            baudrate_known = true;
            break;
        }
    }
    if(!baudrate_known) file->baudrate = 0;

    if(file->framing_src >= UsbUartFramingSrcCount) file->framing_src = UsbUartFramingSrcMenu;
    if(file->data_bits >= UsbUartDataBitsCount) file->data_bits = UsbUartDataBits8;
    if(file->parity >= UsbUartParityCount) file->parity = UsbUartParityNone;
    if(file->stop_bits >= UsbUartStopBitsCount) file->stop_bits = UsbUartStopBits1;
    // Each value in range but not a combination the selected UART supports
    if(!usb_uart_framing_is_valid(file->uart_ch, file->data_bits, file->parity, file->stop_bits)) {
        file->data_bits = UsbUartDataBits8;
        file->parity = UsbUartParityNone;
        file->stop_bits = UsbUartStopBits1;
    }

    if(file->flow_invert > UsbUartFlowInvertBoth) file->flow_invert = UsbUartFlowInvertNone;

    memset(file->reserved, 0, sizeof(file->reserved));
}

static void usb_uart_settings_from_config(UsbUartSettingsFile* file, const UsbUartConfig* cfg) {
    memset(file, 0, sizeof(*file));
    file->baudrate = cfg->baudrate;
    file->vcp_ch = cfg->vcp_ch;
    file->uart_ch = cfg->uart_ch;
    file->flow_pins = cfg->flow_pins;
    file->software_de_re = cfg->software_de_re;
    file->framing_src = cfg->framing_src;
    file->data_bits = cfg->data_bits;
    file->parity = cfg->parity;
    file->stop_bits = cfg->stop_bits;
    file->flow_invert = cfg->flow_invert;
    usb_uart_settings_sanitize(file);
}

// Reads the settings file. Returns false and all-default (zero) settings if
// there is no usable file; exists tells a missing file from a broken one.
static bool usb_uart_settings_read(UsbUartSettingsFile* file, bool* exists) {
    memset(file, 0, sizeof(*file));

    // Checked first: a missing file is the normal case and not worth an error log
    Storage* storage = furi_record_open(RECORD_STORAGE);
    *exists = storage_file_exists(storage, USB_UART_SETTINGS_PATH);
    furi_record_close(RECORD_STORAGE);
    if(!*exists) return false;

    // Magic, version, size and checksum are verified by saved_struct, which
    // leaves the buffer untouched on any mismatch
    const bool loaded = saved_struct_load(
        USB_UART_SETTINGS_PATH,
        file,
        sizeof(*file),
        USB_UART_SETTINGS_MAGIC,
        USB_UART_SETTINGS_VERSION);

    usb_uart_settings_sanitize(file);
    return loaded;
}

bool usb_uart_settings_load(UsbUartConfig* cfg) {
    furi_check(cfg);

    UsbUartSettingsFile file;
    bool exists;
    const bool loaded = usb_uart_settings_read(&file, &exists);

    memset(cfg, 0, sizeof(*cfg));
    cfg->vcp_ch = file.vcp_ch;
    cfg->uart_ch = file.uart_ch;
    cfg->flow_pins = file.flow_pins;
    cfg->software_de_re = file.software_de_re;
    cfg->framing_src = file.framing_src;
    cfg->data_bits = file.data_bits;
    cfg->parity = file.parity;
    cfg->stop_bits = file.stop_bits;
    cfg->flow_invert = file.flow_invert;

    // baudrate_mode is the menu index: 0 = host, 1.. = usb_uart_baudrate_list
    cfg->baudrate = file.baudrate;
    cfg->baudrate_mode = 0;
    for(size_t i = 0; i < USB_UART_BAUDRATE_COUNT; i++) {
        if(file.baudrate == usb_uart_baudrate_list[i]) {
            cfg->baudrate_mode = (uint8_t)(i + 1);
            break;
        }
    }

    return loaded;
}

bool usb_uart_settings_save(const UsbUartConfig* cfg) {
    furi_check(cfg);

    UsbUartSettingsFile file;
    usb_uart_settings_from_config(&file, cfg);

    // Nothing to write if a load already returns exactly this: either from a
    // valid file, or as the defaults while there is no file at all. A broken
    // file is always replaced.
    UsbUartSettingsFile stored;
    bool exists;
    const bool stored_valid = usb_uart_settings_read(&stored, &exists);
    if((stored_valid || !exists) && memcmp(&file, &stored, sizeof(file)) == 0) return true;

    return saved_struct_save(
        USB_UART_SETTINGS_PATH,
        &file,
        sizeof(file),
        USB_UART_SETTINGS_MAGIC,
        USB_UART_SETTINGS_VERSION);
}
