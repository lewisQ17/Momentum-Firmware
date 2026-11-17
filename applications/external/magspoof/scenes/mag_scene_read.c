// Creator: Hummus@FlipperGang

#include "../mag_i.h"
#include "../helpers/mag_helpers.h"

#include "mag_scene_read.h"

#define TAG "MagSceneRead"

// Guards uart_text_box_store, shared between the UART worker and the GUI thread
static FuriMutex* uart_store_mutex = NULL;

void uart_callback(FuriHalSerialHandle* handle, FuriHalSerialRxEvent event, void* context) {
    Mag* mag = context;
    if(event == FuriHalSerialRxEventData) {
        uint8_t data = furi_hal_serial_async_rx(handle);
        furi_stream_buffer_send(mag->uart_rx_stream, &data, 1, 0);
        furi_thread_flags_set(furi_thread_get_id(mag->uart_rx_thread), WorkerEvtRxDone);
    }
}

static int32_t uart_worker(void* context) {
    Mag* mag = context;
    // uart_rx_stream is allocated in on_enter, before RX starts
    mag->uart_text_box_store_strlen = 0;

    while(1) {
        uint32_t events =
            furi_thread_flags_wait(WORKER_ALL_RX_EVENTS, FuriFlagWaitAny, FuriWaitForever);
        // furi_check((events & FuriFlagError) == 0);

        if(events & WorkerEvtStop) break;
        if(events & WorkerEvtRxDone) {
            FURI_LOG_D(TAG, "WorkerEvtRxDone");
            // notification_message(mag->notifications, &sequence_success);
            size_t len = furi_stream_buffer_receive(
                mag->uart_rx_stream, mag->uart_rx_buf, UART_RX_BUF_SIZE, 200);
            FURI_LOG_D(TAG, "UART RX len: %d", len);

            if(len > 0) {
                furi_mutex_acquire(uart_store_mutex, FuriWaitForever);
                // If text box store gets too big, then truncate it
                mag->uart_text_box_store_strlen += len;

                if(mag->uart_text_box_store_strlen >= UART_TERMINAL_TEXT_BOX_STORE_SIZE - 1) {
                    furi_string_right(
                        mag->uart_text_box_store, mag->uart_text_box_store_strlen / 2);
                    mag->uart_text_box_store_strlen =
                        furi_string_size(mag->uart_text_box_store) + len;
                }

                // Add '\0' to the end of the string, and then add the new data
                mag->uart_rx_buf[len] = '\0';
                furi_string_cat_printf(mag->uart_text_box_store, "%s", mag->uart_rx_buf);

                FURI_LOG_D(TAG, "UART RX buf: %*.s", len, mag->uart_rx_buf);
                FURI_LOG_D(
                    TAG, "UART RX store: %s", furi_string_get_cstr(mag->uart_text_box_store));
                furi_mutex_release(uart_store_mutex);
            }

            FURI_LOG_D(TAG, "UARTEventRxData");

            view_dispatcher_send_custom_event(mag->view_dispatcher, UARTEventRxData);
        }
    }

    return 0;
}

void update_widgets(Mag* mag) {
    // Clear widget from all elements
    widget_reset(mag->widget);

    // Titlebar
    widget_add_icon_element(mag->widget, 38, -1, &I_mag_file_10px);
    widget_add_string_element(mag->widget, 64, 0, AlignCenter, AlignTop, FontPrimary, "READ");
    widget_add_icon_element(mag->widget, 81, -1, &I_mag_file_10px);

    // Text box
    furi_mutex_acquire(uart_store_mutex, FuriWaitForever);
    widget_add_text_scroll_element(
        mag->widget, 0, 10, 128, 40, furi_string_get_cstr(mag->uart_text_box_store));
    furi_mutex_release(uart_store_mutex);

    // Buttons
    widget_add_button_element(mag->widget, GuiButtonTypeLeft, "Clear", mag_widget_callback, mag);
    widget_add_button_element(mag->widget, GuiButtonTypeRight, "Parse", mag_widget_callback, mag);
}

void mag_scene_read_on_enter(void* context) {
    Mag* mag = context;
    uart_store_mutex = furi_mutex_alloc(FuriMutexTypeNormal);
    FuriString* message = furi_string_alloc();
    furi_string_printf(message, "Please swipe a card!\n");
    mag->uart_text_box_store = message;

    view_dispatcher_switch_to_view(mag->view_dispatcher, MagViewWidget);

    update_widgets(mag);

    // Allocate the stream and start the worker first: the RX interrupt uses both
    mag->uart_rx_stream = furi_stream_buffer_alloc(UART_RX_BUF_SIZE, 1);

    mag->uart_rx_thread = furi_thread_alloc();
    furi_thread_set_name(mag->uart_rx_thread, "UartRx");
    furi_thread_set_stack_size(mag->uart_rx_thread, 1024);
    furi_thread_set_context(mag->uart_rx_thread, mag);
    furi_thread_set_callback(mag->uart_rx_thread, uart_worker);

    furi_thread_start(mag->uart_rx_thread);

    // Initialize UART
    mag->serial_handle = furi_hal_serial_control_acquire(FuriHalSerialIdUsart);
    furi_check(mag->serial_handle);
    furi_hal_serial_init(mag->serial_handle, 9600);
    furi_hal_serial_async_rx_start(mag->serial_handle, uart_callback, mag, false);
    FURI_LOG_D(TAG, "UART initialized");
    FURI_LOG_D(TAG, "UART worker started");
}

bool mag_scene_read_on_event(void* context, SceneManagerEvent event) {
    Mag* mag = context;

    bool consumed = false;
    bool left_scene = false; // on_exit already freed the scene state

    if(event.type == SceneManagerEventTypeCustom) {
        FURI_LOG_D(TAG, "Custom event: %ld", event.event);

        switch(event.event) {
        case GuiButtonTypeLeft: // Clear
            consumed = true;
            // Clear text box store
            furi_mutex_acquire(uart_store_mutex, FuriWaitForever);
            furi_string_reset(mag->uart_text_box_store);
            mag->uart_text_box_store_strlen = 0;
            furi_mutex_release(uart_store_mutex);
            break;

        case GuiButtonTypeRight: // Parse
            consumed = true;
            FURI_LOG_D(TAG, "Trying to parse");
            MagDevice* mag_dev = mag->mag_dev;

            furi_mutex_acquire(uart_store_mutex, FuriWaitForever);
            bool res = mag_device_parse_card_string(mag_dev, mag->uart_text_box_store);
            furi_string_reset(mag->uart_text_box_store);
            if(res) {
                notification_message(mag->notifications, &sequence_success);

                furi_string_printf(
                    mag->uart_text_box_store,
                    "Track 1: %.*s\nTrack 2: %.*s\nTrack 3: %.*s",
                    mag_dev->dev_data.track[0].len,
                    furi_string_get_cstr(mag_dev->dev_data.track[0].str),
                    mag_dev->dev_data.track[1].len,
                    furi_string_get_cstr(mag_dev->dev_data.track[1].str),
                    mag_dev->dev_data.track[2].len,
                    furi_string_get_cstr(mag_dev->dev_data.track[2].str));
                furi_mutex_release(uart_store_mutex);

                // Switch to saved menu scene (this runs our on_exit)
                left_scene = true;
                scene_manager_next_scene(mag->scene_manager, MagSceneSavedMenu);

            } else {
                furi_string_printf(mag->uart_text_box_store, "Failed to parse! Try again\n");
                notification_message(mag->notifications, &sequence_error);
                furi_mutex_release(uart_store_mutex);
            }

            break;
        }

        if(!left_scene) update_widgets(mag);
    }

    return consumed;
}

void mag_scene_read_on_exit(void* context) {
    Mag* mag = context;
    // notification_message(mag->notifications, &sequence_blink_stop);
    widget_reset(mag->widget);
    // view_dispatcher_remove_view(mag->view_dispatcher, MagViewWidget);

    // Stop UART worker
    FURI_LOG_D(TAG, "Stopping UART worker");
    furi_hal_serial_async_rx_stop(mag->serial_handle);
    furi_hal_serial_deinit(mag->serial_handle);
    furi_hal_serial_control_release(mag->serial_handle);

    furi_thread_flags_set(furi_thread_get_id(mag->uart_rx_thread), WorkerEvtStop);
    furi_thread_join(mag->uart_rx_thread);
    furi_thread_free(mag->uart_rx_thread);
    mag->uart_rx_thread = NULL;
    furi_stream_buffer_free(mag->uart_rx_stream);
    mag->uart_rx_stream = NULL;
    FURI_LOG_D(TAG, "UART worker stopped");

    furi_string_free(mag->uart_text_box_store);
    mag->uart_text_box_store = NULL;
    furi_mutex_free(uart_store_mutex);
    uart_store_mutex = NULL;

    notification_message(mag->notifications, &sequence_blink_stop);
}
