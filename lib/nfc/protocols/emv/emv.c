//#include "emv_i.h"

#include "flipper_format.h"
#include <core/common_defines.h>
#include "protocols/emv/emv.h"
#include <furi.h>
#include <stdlib.h>
#include <string.h>

#define EMV_PROTOCOL_NAME "EMV"

const NfcDeviceBase nfc_device_emv = {
    .protocol_name = EMV_PROTOCOL_NAME,
    .alloc = (NfcDeviceAlloc)emv_alloc,
    .free = (NfcDeviceFree)emv_free,
    .reset = (NfcDeviceReset)emv_reset,
    .copy = (NfcDeviceCopy)emv_copy,
    .verify = (NfcDeviceVerify)emv_verify,
    .load = (NfcDeviceLoad)emv_load,
    .save = (NfcDeviceSave)emv_save,
    .is_equal = (NfcDeviceEqual)emv_is_equal,
    .get_name = (NfcDeviceGetName)emv_get_device_name,
    .get_uid = (NfcDeviceGetUid)emv_get_uid,
    .set_uid = (NfcDeviceSetUid)emv_set_uid,
    .get_base_data = (NfcDeviceGetBaseData)emv_get_base_data,
};

EmvData* emv_alloc(void) {
    EmvData* data = malloc(sizeof(EmvData));
    data->iso14443_4a_data = iso14443_4a_alloc();
    // malloc does not clear memory, and several EmvApplication fields are
    // rendered/saved even when a card omits their tag. Zero it up-front (same as
    // emv_reset) so the poller's per-field stale-data guards are defense-in-depth.
    memset(&data->emv_application, 0, sizeof(EmvApplication));
    data->emv_application.pin_try_counter = 0xff;

    return data;
}

void emv_free(EmvData* data) {
    furi_assert(data);

    emv_reset(data);
    iso14443_4a_free(data->iso14443_4a_data);
    free(data);
}

void emv_reset(EmvData* data) {
    furi_assert(data);

    iso14443_4a_reset(data->iso14443_4a_data);

    memset(&data->emv_application, 0, sizeof(EmvApplication));
}

void emv_copy(EmvData* destination, const EmvData* source) {
    furi_assert(destination);
    furi_assert(source);

    emv_reset(destination);

    iso14443_4a_copy(destination->iso14443_4a_data, source->iso14443_4a_data);
    destination->emv_application = source->emv_application;
}

bool emv_verify(EmvData* data, const FuriString* device_type) {
    UNUSED(data);
    return furi_string_equal_str(device_type, EMV_PROTOCOL_NAME);
}

// Read a "<len_key>" uint32 length followed by its "<hex_key>" hex blob into a
// fixed-size buffer, rejecting the whole file if the declared length exceeds the
// destination capacity (guards against a heap overflow from a malformed/malicious
// .nfc file). Shared by the PAN and AID load paths.
static bool emv_load_len_prefixed_hex(
    FlipperFormat* ff,
    const char* len_key,
    const char* hex_key,
    uint8_t* dst,
    size_t dst_cap,
    uint8_t* out_len) {
    uint32_t len;
    if(!flipper_format_read_uint32(ff, len_key, &len, 1)) return false;
    if(len > dst_cap) return false;
    if(!flipper_format_read_hex(ff, hex_key, dst, len)) return false;
    *out_len = (uint8_t)len;
    return true;
}

bool emv_load(EmvData* data, FlipperFormat* ff, uint32_t version) {
    furi_assert(data);

    FuriString* temp_str = furi_string_alloc();
    bool parsed = false;

    do {
        // Read ISO14443_4A data
        if(!iso14443_4a_load(data->iso14443_4a_data, ff, version)) break;

        EmvApplication* app = &data->emv_application;

        // The following strings come from an externally supplied .nfc file and
        // must never overflow their fixed-size destination buffers. Use a
        // length-bounded copy with explicit NUL termination (truncates on
        // over-long input instead of overflowing the heap-allocated struct).
        flipper_format_read_string(ff, "Cardholder name", temp_str);
        strncpy(
            app->cardholder_name,
            furi_string_get_cstr(temp_str),
            sizeof(app->cardholder_name) - 1);
        app->cardholder_name[sizeof(app->cardholder_name) - 1] = '\0';

        flipper_format_read_string(ff, "Application name", temp_str);
        strncpy(
            app->application_name,
            furi_string_get_cstr(temp_str),
            sizeof(app->application_name) - 1);
        app->application_name[sizeof(app->application_name) - 1] = '\0';

        flipper_format_read_string(ff, "Application label", temp_str);
        strncpy(
            app->application_label,
            furi_string_get_cstr(temp_str),
            sizeof(app->application_label) - 1);
        app->application_label[sizeof(app->application_label) - 1] = '\0';

        // PAN: max sizeof(app->pan) (10 = 19 BCD digits); AID: max sizeof(app->aid)
        // (16) per EMV. The helper rejects the file if the declared length exceeds
        // the buffer, preventing a heap overflow write.
        if(!emv_load_len_prefixed_hex(
               ff, "PAN length", "PAN", app->pan, sizeof(app->pan), &app->pan_len))
            break;

        if(!emv_load_len_prefixed_hex(
               ff, "AID length", "AID", app->aid, sizeof(app->aid), &app->aid_len))
            break;

        if(!flipper_format_read_hex(
               ff, "Application interchange profile", app->application_interchange_profile, 2))
            break;

        if(!flipper_format_read_hex(ff, "Country code", (uint8_t*)&app->country_code, 2)) break;

        if(!flipper_format_read_hex(ff, "Currency code", (uint8_t*)&app->currency_code, 2)) break;

        if(!flipper_format_read_hex(ff, "Expiration year", &app->exp_year, 1)) break;
        if(!flipper_format_read_hex(ff, "Expiration month", &app->exp_month, 1)) break;
        if(!flipper_format_read_hex(ff, "Expiration day", &app->exp_day, 1)) break;

        if(!flipper_format_read_hex(ff, "Effective year", &app->effective_year, 1)) break;
        if(!flipper_format_read_hex(ff, "Effective month", &app->effective_month, 1)) break;
        if(!flipper_format_read_hex(ff, "Effective day", &app->effective_day, 1)) break;

        uint32_t pin_try_counter;
        if(!flipper_format_read_uint32(ff, "PIN try counter", &pin_try_counter, 1)) break;
        app->pin_try_counter = pin_try_counter;

        parsed = true;
    } while(false);

    furi_string_free(temp_str);

    return parsed;
}

bool emv_save(const EmvData* data, FlipperFormat* ff) {
    furi_assert(data);

    FuriString* temp_str = furi_string_alloc();
    bool saved = false;

    do {
        EmvApplication app = data->emv_application;
        if(!iso14443_4a_save(data->iso14443_4a_data, ff)) break;

        if(!flipper_format_write_comment_cstr(ff, "EMV specific data:\n")) break;

        if(!flipper_format_write_string_cstr(ff, "Cardholder name", app.cardholder_name)) break;

        if(!flipper_format_write_string_cstr(ff, "Application name", app.application_name)) break;

        if(!flipper_format_write_string_cstr(ff, "Application label", app.application_label))
            break;

        uint32_t pan_len = app.pan_len;
        if(!flipper_format_write_uint32(ff, "PAN length", &pan_len, 1)) break;

        if(!flipper_format_write_hex(ff, "PAN", app.pan, pan_len)) break;

        uint32_t aid_len = app.aid_len;
        if(!flipper_format_write_uint32(ff, "AID length", &aid_len, 1)) break;

        if(!flipper_format_write_hex(ff, "AID", app.aid, aid_len)) break;

        if(!flipper_format_write_hex(
               ff, "Application interchange profile", app.application_interchange_profile, 2))
            break;

        if(!flipper_format_write_hex(ff, "Country code", (uint8_t*)&app.country_code, 2)) break;

        if(!flipper_format_write_hex(ff, "Currency code", (uint8_t*)&app.currency_code, 2)) break;

        if(!flipper_format_write_hex(ff, "Expiration year", (uint8_t*)&app.exp_year, 1)) break;
        if(!flipper_format_write_hex(ff, "Expiration month", (uint8_t*)&app.exp_month, 1)) break;
        if(!flipper_format_write_hex(ff, "Expiration day", (uint8_t*)&app.exp_day, 1)) break;

        if(!flipper_format_write_hex(ff, "Effective year", (uint8_t*)&app.effective_year, 1))
            break;
        if(!flipper_format_write_hex(ff, "Effective month", (uint8_t*)&app.effective_month, 1))
            break;
        if(!flipper_format_write_hex(ff, "Effective day", (uint8_t*)&app.effective_day, 1)) break;

        if(!flipper_format_write_uint32(ff, "PIN try counter", (uint32_t*)&app.pin_try_counter, 1))
            break;

        saved = true;
    } while(false);

    furi_string_free(temp_str);

    return saved;
}

bool emv_is_equal(const EmvData* data, const EmvData* other) {
    furi_assert(data);
    furi_assert(other);

    return iso14443_4a_is_equal(data->iso14443_4a_data, other->iso14443_4a_data) &&
           memcmp(&data->emv_application, &other->emv_application, sizeof(EmvApplication)) == 0;
}

const char* emv_get_device_name(const EmvData* data, NfcDeviceNameType name_type) {
    UNUSED(data);
    UNUSED(name_type);
    return EMV_PROTOCOL_NAME;
}

const uint8_t* emv_get_uid(const EmvData* data, size_t* uid_len) {
    furi_assert(data);

    return iso14443_4a_get_uid(data->iso14443_4a_data, uid_len);
}

bool emv_set_uid(EmvData* data, const uint8_t* uid, size_t uid_len) {
    furi_assert(data);

    return iso14443_4a_set_uid(data->iso14443_4a_data, uid, uid_len);
}

Iso14443_4aData* emv_get_base_data(const EmvData* data) {
    furi_assert(data);

    return data->iso14443_4a_data;
}
