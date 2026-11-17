#include "badkb_script_maker_i.h"

/* Keep the user-facing limit texts below in sync */
_Static_assert(BKSM_MAX_LINES == 200, "update limit text");
_Static_assert(BKSM_MAX_LINE_LEN == 160, "update limit text");
_Static_assert(BKSM_MAX_SCRIPT_SIZE == 12 * 1024, "update limit text");

void bksm_script_clear(BksmScript* script) {
    for(uint16_t i = 0; i < script->count; i++) {
        free(script->lines[i]);
        script->lines[i] = NULL;
    }
    script->count = 0;
    script->bytes = 0;
}

static char* bksm_strdup_bounded(const char* text, size_t len) {
    char* copy = malloc(len + 1);
    memcpy(copy, text, len);
    copy[len] = '\0';
    return copy;
}

BksmScriptResult bksm_script_insert(BksmScript* script, uint16_t index, const char* text) {
    size_t len = strnlen(text, BKSM_MAX_LINE_LEN + 1);
    if(len > BKSM_MAX_LINE_LEN) return BksmScriptLineTooLong;
    if(script->count >= BKSM_MAX_LINES) return BksmScriptTooManyLines;
    if(script->bytes + len + 1 > BKSM_MAX_SCRIPT_SIZE) return BksmScriptTooLarge;
    if(index > script->count) index = script->count;

    memmove(
        &script->lines[index + 1],
        &script->lines[index],
        (script->count - index) * sizeof(script->lines[0]));
    script->lines[index] = bksm_strdup_bounded(text, len);
    script->count++;
    script->bytes += len + 1;
    return BksmScriptOk;
}

BksmScriptResult bksm_script_replace(BksmScript* script, uint16_t index, const char* text) {
    if(index >= script->count) return bksm_script_insert(script, index, text);

    size_t len = strnlen(text, BKSM_MAX_LINE_LEN + 1);
    if(len > BKSM_MAX_LINE_LEN) return BksmScriptLineTooLong;
    size_t old_len = strlen(script->lines[index]);
    if(script->bytes - old_len + len > BKSM_MAX_SCRIPT_SIZE) return BksmScriptTooLarge;

    char* copy = bksm_strdup_bounded(text, len);
    free(script->lines[index]);
    script->lines[index] = copy;
    script->bytes = script->bytes - old_len + len;
    return BksmScriptOk;
}

void bksm_script_delete(BksmScript* script, uint16_t index) {
    if(index >= script->count) return;
    script->bytes -= strlen(script->lines[index]) + 1;
    free(script->lines[index]);
    memmove(
        &script->lines[index],
        &script->lines[index + 1],
        (script->count - index - 1) * sizeof(script->lines[0]));
    script->count--;
    script->lines[script->count] = NULL;
}

void bksm_script_swap(BksmScript* script, uint16_t a, uint16_t b) {
    if(a >= script->count || b >= script->count || a == b) return;
    char* tmp = script->lines[a];
    script->lines[a] = script->lines[b];
    script->lines[b] = tmp;
}

const char* bksm_script_result_text(BksmScriptResult result) {
    switch(result) {
    case BksmScriptTooManyLines:
        return "Line limit reached\n(max 200 lines)";
    case BksmScriptLineTooLong:
        return "Line too long\n(max 160 chars)";
    case BksmScriptTooLarge:
        return "Script size limit\nreached (12 KB)";
    default:
        return "OK";
    }
}
