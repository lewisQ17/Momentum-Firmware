#include "iso14443_3a_render.h"

void nfc_render_iso14443_3a_format_bytes(FuriString* str, const uint8_t* data, size_t size) {
    for(size_t i = 0; i < size; i++) {
        furi_string_cat_printf(str, " %02X", data[i]);
    }
}

void nfc_render_iso14443_tech_type(const Iso14443_3aData* data, FuriString* str) {
    const char iso_type = iso14443_3a_supports_iso14443_4(data) ? '4' : '3';
    furi_string_cat_printf(str, "Tech: ISO 14443-%c (NFC-A)\n", iso_type);
}

void nfc_render_iso14443_3a_info(
    const Iso14443_3aData* data,
    NfcProtocolFormatType format_type,
    FuriString* str) {
    if(format_type == NfcProtocolFormatTypeFull) {
        nfc_render_iso14443_tech_type(data, str);
    }

    nfc_render_iso14443_3a_brief(data, str);

    if(format_type == NfcProtocolFormatTypeFull) {
        nfc_render_iso14443_3a_extra(data, str);
    }
}

void nfc_render_iso14443_3a_brief(const Iso14443_3aData* data, FuriString* str) {
    furi_string_cat_printf(str, "UID:");

    nfc_render_iso14443_3a_format_bytes(str, data->uid, data->uid_len);
}

void nfc_render_iso14443_3a_extra(const Iso14443_3aData* data, FuriString* str) {
    furi_string_cat_printf(str, "\nATQA: %02X %02X  ", data->atqa[1], data->atqa[0]);
    furi_string_cat_printf(str, "\nSAK: %02X", data->sak);

    // Access-control systems print the same UID in different forms. Surface the
    // reversed (LSB-first) byte order and the decimal card number (MSB- and
    // LSB-first) so no manual byte-swapping is needed during badge work.
    furi_string_cat_printf(str, "\nUID (rev):");
    for(size_t i = data->uid_len; i > 0; i--) {
        furi_string_cat_printf(str, " %02X", data->uid[i - 1]);
    }
    if(data->uid_len <= 8) {
        uint64_t be = 0, le = 0;
        for(size_t i = 0; i < data->uid_len; i++) {
            be = (be << 8) | data->uid[i];
            le |= (uint64_t)data->uid[i] << (8 * i);
        }
        furi_string_cat_printf(str, "\nUID dec: %llu", be);
        furi_string_cat_printf(str, "\nUID dec (rev): %llu", le);
    }
}
