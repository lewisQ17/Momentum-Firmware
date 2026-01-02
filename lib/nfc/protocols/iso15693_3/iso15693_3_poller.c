#include "iso15693_3_poller_i.h"

#include <nfc/protocols/nfc_poller_base.h>

#include <furi.h>

#define TAG "ISO15693_3Poller"

typedef NfcCommand (*Iso15693_3PollerStateHandler)(Iso15693_3Poller* instance);

const Iso15693_3Data* iso15693_3_poller_get_data(Iso15693_3Poller* instance) {
    furi_assert(instance);
    furi_assert(instance->data);

    return instance->data;
}

static Iso15693_3Poller* iso15693_3_poller_alloc(Nfc* nfc) {
    furi_assert(nfc);

    Iso15693_3Poller* instance = malloc(sizeof(Iso15693_3Poller));
    instance->nfc = nfc;
    instance->tx_buffer = bit_buffer_alloc(ISO15693_3_POLLER_MAX_BUFFER_SIZE);
    instance->rx_buffer = bit_buffer_alloc(ISO15693_3_POLLER_MAX_BUFFER_SIZE);

    nfc_config(instance->nfc, NfcModePoller, NfcTechIso15693);
    nfc_set_guard_time_us(instance->nfc, ISO15693_3_GUARD_TIME_US);
    nfc_set_fdt_poll_fc(instance->nfc, ISO15693_3_FDT_POLL_FC);
    nfc_set_fdt_poll_poll_us(instance->nfc, ISO15693_3_POLL_POLL_MIN_US);
    instance->data = iso15693_3_alloc();

    instance->iso15693_3_event.data = &instance->iso15693_3_event_data;
    instance->general_event.protocol = NfcProtocolIso15693_3;
    instance->general_event.event_data = &instance->iso15693_3_event;
    instance->general_event.instance = instance;

    return instance;
}

static void iso15693_3_poller_free(Iso15693_3Poller* instance) {
    furi_assert(instance);

    furi_assert(instance->tx_buffer);
    furi_assert(instance->rx_buffer);
    furi_assert(instance->data);

    bit_buffer_free(instance->tx_buffer);
    bit_buffer_free(instance->rx_buffer);
    iso15693_3_free(instance->data);
    free(instance);
}

static NfcCommand iso15693_3_poller_handler_idle(Iso15693_3Poller* instance) {
    bit_buffer_reset(instance->tx_buffer);
    bit_buffer_reset(instance->rx_buffer);

    instance->state = Iso15693_3PollerStateRequestMode;
    return NfcCommandContinue;
}

static NfcCommand iso15693_3_poller_handler_request_mode(Iso15693_3Poller* instance) {
    NfcCommand command = NfcCommandContinue;

    instance->iso15693_3_event.type = Iso15693_3PollerEventTypeRequestMode;
    instance->iso15693_3_event.data->poller_mode.mode = Iso15693_3PollerModeRead;
    instance->iso15693_3_event.data->poller_mode.data = NULL;

    command = instance->callback(instance->general_event, instance->context);
    instance->mode = instance->iso15693_3_event.data->poller_mode.mode;
    if(instance->mode == Iso15693_3PollerModeWrite) {
        iso15693_3_copy(instance->data, instance->iso15693_3_event.data->poller_mode.data);
    }

    instance->state = Iso15693_3PollerStateActivate;
    return command;
}

static NfcCommand iso15693_3_poller_handler_failed(Iso15693_3Poller* instance) {
    FURI_LOG_D(TAG, "Operation Failed");
    instance->iso15693_3_event.type = instance->mode == Iso15693_3PollerModeRead ?
                                          Iso15693_3PollerEventTypeReadFailed :
                                          Iso15693_3PollerEventTypeWriteFailed;
    instance->iso15693_3_event.data->error = instance->error;
    NfcCommand command = instance->callback(instance->general_event, instance->context);
    instance->state = Iso15693_3PollerStateIdle;
    return command;
}

static NfcCommand iso15693_3_poller_handler_success(Iso15693_3Poller* instance) {
    FURI_LOG_D(TAG, "Operation succeeded");
    instance->iso15693_3_event.type = instance->mode == Iso15693_3PollerModeRead ?
                                          Iso15693_3PollerEventTypeReadSuccess :
                                          Iso15693_3PollerEventTypeWriteSuccess;
    NfcCommand command = instance->callback(instance->general_event, instance->context);
    return command;
}

static const Iso15693_3PollerStateHandler
    iso15693_3_poller_read_handler[Iso15693_3PollerStateNum] = {
        [Iso15693_3PollerStateIdle] = iso15693_3_poller_handler_idle,
        [Iso15693_3PollerStateRequestMode] = iso15693_3_poller_handler_request_mode,
        [Iso15693_3PollerStateFailed] = iso15693_3_poller_handler_failed,
        [Iso15693_3PollerStateSuccess] = iso15693_3_poller_handler_success,
};

static void iso15693_3_poller_set_callback(
    Iso15693_3Poller* instance,
    NfcGenericCallback callback,
    void* context) {
    furi_assert(instance);
    furi_assert(callback);

    instance->callback = callback;
    instance->context = context;
}

static NfcCommand iso15693_3_poller_run(NfcGenericEvent event, void* context) {
    furi_assert(context);
    furi_assert(event.protocol == NfcProtocolInvalid);
    furi_assert(event.event_data);

    Iso15693_3Poller* instance = context;
    NfcEvent* nfc_event = event.event_data;
    NfcCommand command = NfcCommandContinue;

    if(nfc_event->type == NfcEventTypePollerReady) {
        command = iso15693_3_poller_read_handler[instance->state](instance);

        // FIXME: convert to new state machine
        if(instance->state != Iso15693_3PollerStateActivated) {
            Iso15693_3Error error = iso15693_3_poller_activate(instance, instance->data);
            if(error == Iso15693_3ErrorNone) {
                instance->iso15693_3_event.type = Iso15693_3PollerEventTypeReady;
                instance->iso15693_3_event_data.error = error;
                command = instance->callback(instance->general_event, instance->context);
            } else {
                instance->iso15693_3_event.type = Iso15693_3PollerEventTypeError;
                instance->iso15693_3_event_data.error = error;
                command = instance->callback(instance->general_event, instance->context);
                // Add delay to switch context
                furi_delay_ms(100);
            }
        } else {
            instance->iso15693_3_event.type = Iso15693_3PollerEventTypeReady;
            instance->iso15693_3_event_data.error = Iso15693_3ErrorNone;
            command = instance->callback(instance->general_event, instance->context);
        }
    }

    if(command == NfcCommandReset) {
        instance->state = Iso15693_3PollerStateIdle;
    }

    return command;
}

static bool iso15693_3_poller_detect(NfcGenericEvent event, void* context) {
    furi_assert(context);
    furi_assert(event.event_data);
    furi_assert(event.instance);
    furi_assert(event.protocol == NfcProtocolInvalid);

    bool protocol_detected = false;
    Iso15693_3Poller* instance = context;
    NfcEvent* nfc_event = event.event_data;
    furi_assert(instance->state == Iso15693_3PollerStateIdle);

    if(nfc_event->type == NfcEventTypePollerReady) {
        uint8_t uid[ISO15693_3_UID_SIZE];
        Iso15693_3Error error = iso15693_3_poller_inventory(instance, uid);
        protocol_detected = (error == Iso15693_3ErrorNone);
    }

    return protocol_detected;
}

const NfcPollerBase nfc_poller_iso15693_3 = {
    .alloc = (NfcPollerAlloc)iso15693_3_poller_alloc,
    .free = (NfcPollerFree)iso15693_3_poller_free,
    .set_callback = (NfcPollerSetCallback)iso15693_3_poller_set_callback,
    .run = (NfcPollerRun)iso15693_3_poller_run,
    .detect = (NfcPollerDetect)iso15693_3_poller_detect,
    .get_data = (NfcPollerGetData)iso15693_3_poller_get_data,
};
