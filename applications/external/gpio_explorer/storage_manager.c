#include "storage_manager.h"

static Storage *storage;

uint8_t configuration_file_init(){
    storage = furi_record_open(RECORD_STORAGE);
    if(storage_common_exists(storage,CONF_FILE_FULL_PATH)){
        return 1;
    }else{
        File *file = storage_file_alloc(storage);
        storage_common_mkdir(storage,CONF_FOLDER_PATH);
        storage_file_open(file,CONF_FILE_FULL_PATH,FSAM_WRITE,FSOM_CREATE_NEW);
        storage_file_close(file);
        storage_file_free(file);
        return 0;
    }
}

uint8_t configuration_file_read(struct gpio_explorer_configure_struct* configuration){
    File *file = storage_file_alloc(storage);
    char *buffer = malloc(sizeof *buffer * READ_BUFFER_SIZE);

    if(!file || !buffer) {
        storage_file_close(file);
        assert(0);
    }

    // An empty/short/hand-edited file must not give out-of-range pin indexes
    bool valid = storage_file_open(file,CONF_FILE_FULL_PATH,FSAM_READ,FSOM_OPEN_EXISTING) &&
        storage_file_read(file,buffer,CONFIG_FILE_SIZE) == CONFIG_FILE_SIZE;
    storage_file_close(file);
    storage_file_free(file);

    if(valid) {
        const uint8_t digit_pos[4] = {11, 24, 37, 48};
        for(uint8_t i = 0; i < 4; i++) {
            if(buffer[digit_pos[i]] < '0' || buffer[digit_pos[i]] > '7') valid = false;
        }
    }

    if(valid) {
        configuration->rgb_pin_r_index = buffer[11]-'0';
        configuration->rgb_pin_g_index = buffer[24]-'0';
        configuration->rgb_pin_b_index = buffer[37]-'0';
        configuration->led_pin_index = buffer[48]-'0';
    } else {
        // same values as DEFAULT_RGB_R/G/B_PIN and DEFAULT_LED_PIN in app.h
        configuration->rgb_pin_r_index = 5;
        configuration->rgb_pin_g_index = 4;
        configuration->rgb_pin_b_index = 3;
        configuration->led_pin_index = 0;
    }
    free(buffer);

    return 1;
}

uint8_t configuration_file_write(struct gpio_explorer_configure_struct* configuration){
    File *file = storage_file_alloc(storage);
    char* buffer = malloc(sizeof *buffer * READ_BUFFER_SIZE);
    if(!buffer||!file){
        storage_file_close(file);
        assert(0);
    }
    __wrap_snprintf(buffer,CONFIG_FILE_SIZE,"RGB_R_PIN: %d\nRGB_G_PIN: %d\nRGB_B_PIN: %d\nLED_PIN: %d",
        configuration->rgb_pin_r_index,
        configuration->rgb_pin_g_index,
        configuration->rgb_pin_b_index,
        configuration->led_pin_index);

    if(storage_file_open(file,CONF_FILE_FULL_PATH,FSAM_WRITE,FSOM_OPEN_EXISTING)) {
        storage_file_write(file,buffer,CONFIG_FILE_SIZE);
    }
    free(buffer);
    storage_file_close(file);
    storage_file_free(file);

    return 1;
}

void free_storage_manager(){
    furi_record_close(RECORD_STORAGE);
}