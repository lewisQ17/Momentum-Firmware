#include "../mag_i.h"
#include <stdio.h>
#include <string.h>

// Capacity of the encode buffers (all callers must use these sizes, mag_encode() stops at the limit)
#define MAG_RAW_BYTES        96
#define MAG_MANCHESTER_BYTES (MAG_RAW_BYTES * 2)

void play_halfbit(bool value, MagState* state);
void play_track(uint8_t* bits_manchester, uint16_t n_bits, MagState* state, bool reverse);

void tx_init_rf(int hz);
void tx_init_rfid();
void tx_init_piezo();
bool tx_init(MagState* state);
void tx_deinit_piezo();
void tx_deinit_rfid();
bool tx_deinit(MagState* state);

uint16_t add_bit(bool value, uint8_t* out, uint16_t count);
uint16_t add_bit_manchester(bool value, uint8_t* out, uint16_t count);
uint16_t mag_encode(
    char* data,
    uint8_t* out_manchester,
    uint8_t* out_raw,
    uint8_t track_bits,
    uint8_t track_ascii_offset);
void debug_mag_string(char* data, uint8_t track_bits, uint8_t track_ascii_offset);
void mag_spoof(Mag* mag);
