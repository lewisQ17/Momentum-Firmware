#include "../usb_uart_bridge.h"
#include "../gpio_app_i.h"
#include <furi_hal.h>

typedef enum {
    UsbUartLineIndexVcp,
    UsbUartLineIndexBaudrate,
    UsbUartLineIndexFraming,
    UsbUartLineIndexDataBits,
    UsbUartLineIndexParity,
    UsbUartLineIndexStopBits,
    UsbUartLineIndexUart,
    UsbUartLineIndexFlow,
    UsbUartLineIndexFlowInvert,
    UsbUartLineIndexDeRe,
} LineIndex;

static const char* const vcp_ch[] = {"0 (CLI)", "1"};
static const char* const uart_ch[] = {"13,14", "15,16"};
static const char* const flow_pins[] = {"None", "2,3", "6,7", "16,15"};
static const char* const baudrate_mode[] = {"Host"};
static const uint32_t baudrate_list[] = {
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
static const char* const software_de_re[] = {"None", "4"};
static const char* const framing_src[UsbUartFramingSrcCount] = {"Menu", "Host"};
static const char* const data_bits[UsbUartDataBitsCount] = {"8", "7", "6"};
static const char* const parity[UsbUartParityCount] = {"None", "Even", "Odd"};
static const char* const stop_bits[UsbUartStopBitsCount] = {"1", "2", "1.5", "0.5"};
static const char* const flow_invert[] = {"None", "RTS", "DTR", "Both"};
_Static_assert(COUNT_OF(flow_invert) == UsbUartFlowInvertBoth + 1, "flow_invert labels");

bool gpio_scene_usb_uart_cfg_on_event(void* context, SceneManagerEvent event) {
    GpioApp* app = context;
    furi_assert(app);
    if(event.type == SceneManagerEventTypeCustom) {
        if(event.event == GpioUsbUartEventConfigSet) {
            usb_uart_set_config(app->usb_uart_bridge, app->usb_uart_cfg);
            return true;
        }
    }
    return false;
}

void line_ensure_flow_invariant(GpioApp* app) {
    // GPIO pins PC0, PC1 (16,15) are unavailable for RTS/DTR when LPUART is
    // selected. This function enforces that invariant by resetting flow_pins
    // to None if it is configured to 16,15 when LPUART is selected.

    uint8_t available_flow_pins = app->usb_uart_cfg->uart_ch == FuriHalSerialIdLpuart ? 3 : 4;
    VariableItem* item = app->var_item_flow;
    variable_item_set_values_count(item, available_flow_pins);

    if(app->usb_uart_cfg->flow_pins >= available_flow_pins) {
        app->usb_uart_cfg->flow_pins = 0;

        variable_item_set_current_value_index(item, app->usb_uart_cfg->flow_pins);
        variable_item_set_current_value_text(item, flow_pins[app->usb_uart_cfg->flow_pins]);
    }
}

static void line_set_item(VariableItem* item, uint8_t index, const char* const* labels) {
    variable_item_set_current_value_index(item, index);
    variable_item_set_current_value_text(item, labels[index]);
}

static void line_ensure_framing_invariant(GpioApp* app) {
    // LPUART (pins 15,16) only supports whole stop bit lengths
    UsbUartConfig* cfg = app->usb_uart_cfg;
    uint8_t available_stop_bits = cfg->uart_ch == FuriHalSerialIdLpuart ?
                                      USB_UART_LPUART_STOP_BITS_COUNT :
                                      UsbUartStopBitsCount;
    variable_item_set_values_count(app->var_item_stop_bits, available_stop_bits);

    if(cfg->stop_bits >= available_stop_bits) {
        cfg->stop_bits = UsbUartStopBits1;
        line_set_item(app->var_item_stop_bits, cfg->stop_bits, stop_bits);
    }
}

static void line_vcp_cb(VariableItem* item) {
    GpioApp* app = variable_item_get_context(item);
    furi_assert(app);
    uint8_t index = variable_item_get_current_value_index(item);

    variable_item_set_current_value_text(item, vcp_ch[index]);

    app->usb_uart_cfg->vcp_ch = index;
    view_dispatcher_send_custom_event(app->view_dispatcher, GpioUsbUartEventConfigSet);
}

static void line_port_cb(VariableItem* item) {
    GpioApp* app = variable_item_get_context(item);
    furi_assert(app);
    uint8_t index = variable_item_get_current_value_index(item);

    variable_item_set_current_value_text(item, uart_ch[index]);

    if(index == 0)
        app->usb_uart_cfg->uart_ch = FuriHalSerialIdUsart;
    else if(index == 1)
        app->usb_uart_cfg->uart_ch = FuriHalSerialIdLpuart;

    line_ensure_flow_invariant(app);
    line_ensure_framing_invariant(app);
    view_dispatcher_send_custom_event(app->view_dispatcher, GpioUsbUartEventConfigSet);
}

static void line_framing_src_cb(VariableItem* item) {
    GpioApp* app = variable_item_get_context(item);
    furi_assert(app);
    uint8_t index = variable_item_get_current_value_index(item);

    variable_item_set_current_value_text(item, framing_src[index]);

    app->usb_uart_cfg->framing_src = index;
    view_dispatcher_send_custom_event(app->view_dispatcher, GpioUsbUartEventConfigSet);
}

static void line_data_bits_cb(VariableItem* item) {
    GpioApp* app = variable_item_get_context(item);
    furi_assert(app);
    uint8_t index = variable_item_get_current_value_index(item);

    variable_item_set_current_value_text(item, data_bits[index]);

    app->usb_uart_cfg->data_bits = index;
    // 6 data bits need a parity bit
    if(index == UsbUartDataBits6 && app->usb_uart_cfg->parity == UsbUartParityNone) {
        app->usb_uart_cfg->parity = UsbUartParityEven;
        line_set_item(app->var_item_parity, app->usb_uart_cfg->parity, parity);
    }
    view_dispatcher_send_custom_event(app->view_dispatcher, GpioUsbUartEventConfigSet);
}

static void line_parity_cb(VariableItem* item) {
    GpioApp* app = variable_item_get_context(item);
    furi_assert(app);
    uint8_t index = variable_item_get_current_value_index(item);

    variable_item_set_current_value_text(item, parity[index]);

    app->usb_uart_cfg->parity = index;
    // 6 data bits need a parity bit
    if(index == UsbUartParityNone && app->usb_uart_cfg->data_bits == UsbUartDataBits6) {
        app->usb_uart_cfg->data_bits = UsbUartDataBits7;
        line_set_item(app->var_item_data_bits, app->usb_uart_cfg->data_bits, data_bits);
    }
    view_dispatcher_send_custom_event(app->view_dispatcher, GpioUsbUartEventConfigSet);
}

static void line_stop_bits_cb(VariableItem* item) {
    GpioApp* app = variable_item_get_context(item);
    furi_assert(app);
    uint8_t index = variable_item_get_current_value_index(item);

    variable_item_set_current_value_text(item, stop_bits[index]);

    app->usb_uart_cfg->stop_bits = index;
    view_dispatcher_send_custom_event(app->view_dispatcher, GpioUsbUartEventConfigSet);
}

static void line_flow_invert_cb(VariableItem* item) {
    GpioApp* app = variable_item_get_context(item);
    furi_assert(app);
    uint8_t index = variable_item_get_current_value_index(item);

    variable_item_set_current_value_text(item, flow_invert[index]);

    app->usb_uart_cfg->flow_invert = index;
    view_dispatcher_send_custom_event(app->view_dispatcher, GpioUsbUartEventConfigSet);
}

static void line_software_de_re_cb(VariableItem* item) {
    GpioApp* app = variable_item_get_context(item);
    furi_assert(app);
    uint8_t index = variable_item_get_current_value_index(item);

    variable_item_set_current_value_text(item, software_de_re[index]);

    app->usb_uart_cfg->software_de_re = index;
    view_dispatcher_send_custom_event(app->view_dispatcher, GpioUsbUartEventConfigSet);
}

static void line_flow_cb(VariableItem* item) {
    GpioApp* app = variable_item_get_context(item);
    furi_assert(app);
    uint8_t index = variable_item_get_current_value_index(item);

    variable_item_set_current_value_text(item, flow_pins[index]);

    app->usb_uart_cfg->flow_pins = index;
    view_dispatcher_send_custom_event(app->view_dispatcher, GpioUsbUartEventConfigSet);
}

static void line_baudrate_cb(VariableItem* item) {
    GpioApp* app = variable_item_get_context(item);
    furi_assert(app);
    uint8_t index = variable_item_get_current_value_index(item);

    char br_text[8];

    if(index > 0) {
        snprintf(br_text, 7, "%lu", baudrate_list[index - 1]);
        variable_item_set_current_value_text(item, br_text);
        app->usb_uart_cfg->baudrate = baudrate_list[index - 1];
    } else {
        variable_item_set_current_value_text(item, baudrate_mode[index]);
        app->usb_uart_cfg->baudrate = 0;
    }
    app->usb_uart_cfg->baudrate_mode = index;
    view_dispatcher_send_custom_event(app->view_dispatcher, GpioUsbUartEventConfigSet);
}

void gpio_scene_usb_uart_cfg_on_enter(void* context) {
    GpioApp* app = context;
    furi_assert(app);
    VariableItemList* var_item_list = app->var_item_list;

    app->usb_uart_cfg = malloc(sizeof(UsbUartConfig));
    usb_uart_get_config(app->usb_uart_bridge, app->usb_uart_cfg);

    // Never index the label tables out of bounds, whatever the config holds
    UsbUartConfig* cfg = app->usb_uart_cfg;
    if(cfg->framing_src >= UsbUartFramingSrcCount) cfg->framing_src = UsbUartFramingSrcMenu;
    if(cfg->data_bits >= UsbUartDataBitsCount) cfg->data_bits = UsbUartDataBits8;
    if(cfg->parity >= UsbUartParityCount) cfg->parity = UsbUartParityNone;
    if(cfg->stop_bits >= UsbUartStopBitsCount) cfg->stop_bits = UsbUartStopBits1;
    if(cfg->flow_invert >= COUNT_OF(flow_invert)) cfg->flow_invert = UsbUartFlowInvertNone;
    if(cfg->software_de_re >= COUNT_OF(software_de_re)) cfg->software_de_re = 0;

    VariableItem* item;
    char br_text[8];

    item = variable_item_list_add(var_item_list, "USB Channel", 2, line_vcp_cb, app);
    variable_item_set_current_value_index(item, app->usb_uart_cfg->vcp_ch);
    variable_item_set_current_value_text(item, vcp_ch[app->usb_uart_cfg->vcp_ch]);

    item = variable_item_list_add(
        var_item_list,
        "Baudrate",
        sizeof(baudrate_list) / sizeof(baudrate_list[0]) + 1,
        line_baudrate_cb,
        app);
    variable_item_set_current_value_index(item, app->usb_uart_cfg->baudrate_mode);
    if(app->usb_uart_cfg->baudrate_mode > 0) {
        snprintf(br_text, 7, "%lu", baudrate_list[app->usb_uart_cfg->baudrate_mode - 1]);
        variable_item_set_current_value_text(item, br_text);
    } else {
        variable_item_set_current_value_text(
            item, baudrate_mode[app->usb_uart_cfg->baudrate_mode]);
    }

    item = variable_item_list_add(
        var_item_list, "Framing", COUNT_OF(framing_src), line_framing_src_cb, app);
    line_set_item(item, cfg->framing_src, framing_src);

    item = variable_item_list_add(
        var_item_list, "Data Bits", COUNT_OF(data_bits), line_data_bits_cb, app);
    line_set_item(item, cfg->data_bits, data_bits);
    app->var_item_data_bits = item;

    item = variable_item_list_add(var_item_list, "Parity", COUNT_OF(parity), line_parity_cb, app);
    line_set_item(item, cfg->parity, parity);
    app->var_item_parity = item;

    item = variable_item_list_add(
        var_item_list, "Stop Bits", COUNT_OF(stop_bits), line_stop_bits_cb, app);
    line_set_item(item, cfg->stop_bits, stop_bits);
    app->var_item_stop_bits = item;

    item = variable_item_list_add(var_item_list, "UART Pins", 2, line_port_cb, app);
    variable_item_set_current_value_index(item, app->usb_uart_cfg->uart_ch);
    variable_item_set_current_value_text(item, uart_ch[app->usb_uart_cfg->uart_ch]);

    item = variable_item_list_add(
        var_item_list, "RTS/DTR Pins", COUNT_OF(flow_pins), line_flow_cb, app);
    variable_item_set_current_value_index(item, app->usb_uart_cfg->flow_pins);
    variable_item_set_current_value_text(item, flow_pins[app->usb_uart_cfg->flow_pins]);
    app->var_item_flow = item;
    line_ensure_flow_invariant(app);
    line_ensure_framing_invariant(app);

    item = variable_item_list_add(
        var_item_list, "RTS/DTR Invert", COUNT_OF(flow_invert), line_flow_invert_cb, app);
    line_set_item(item, cfg->flow_invert, flow_invert);

    item = variable_item_list_add(
        var_item_list, "DE/RE Pin", COUNT_OF(software_de_re), line_software_de_re_cb, app);
    variable_item_set_current_value_index(item, app->usb_uart_cfg->software_de_re);
    variable_item_set_current_value_text(item, software_de_re[app->usb_uart_cfg->software_de_re]);

    variable_item_list_set_selected_item(
        var_item_list, scene_manager_get_scene_state(app->scene_manager, GpioAppViewUsbUartCfg));

    view_dispatcher_switch_to_view(app->view_dispatcher, GpioAppViewUsbUartCfg);
}

void gpio_scene_usb_uart_cfg_on_exit(void* context) {
    GpioApp* app = context;
    scene_manager_set_scene_state(
        app->scene_manager,
        GpioAppViewUsbUartCfg,
        variable_item_list_get_selected_item_index(app->var_item_list));
    variable_item_list_reset(app->var_item_list);
    free(app->usb_uart_cfg);
}
