#include "lcd.h"


static void lcd_pulse_enable(LCD *lcd) {
    gpio_put(lcd->en, 1);
    sleep_us(1);
    gpio_put(lcd->en, 0);
    sleep_us(50);
}

static void lcd_write_nibble(LCD *lcd, uint8_t nibble) {
    for (int i = 0; i < 4; i++) {
        gpio_put(lcd->data[i], (nibble >> i) & 1);
    }
    lcd_pulse_enable(lcd);
}

static void lcd_send_byte(LCD *lcd, uint8_t byte, bool is_data) {
    gpio_put(lcd->rs, is_data);
    lcd_write_nibble(lcd, byte >> 4);   // Nibble alto
    lcd_write_nibble(lcd, byte & 0x0F); // Nibble bajo
    sleep_ms(2);
}

// Inicialización del LCD indicando los pines
void lcd_init(LCD *lcd, uint8_t rs, uint8_t en, uint8_t d4, uint8_t d5, uint8_t d6, uint8_t d7) {
    lcd->rs = rs;
    lcd->en = en;
    lcd->data[0] = d4;
    lcd->data[1] = d5;
    lcd->data[2] = d6;
    lcd->data[3] = d7;

    // Configurar pines como salida
    gpio_init(lcd->rs); gpio_set_dir(lcd->rs, true);
    gpio_init(lcd->en); gpio_set_dir(lcd->en, true);
    for (int i = 0; i < 4; i++) {
        gpio_init(lcd->data[i]);
        gpio_set_dir(lcd->data[i], true);
    }

    sleep_ms(50);

    // Secuencia de inicialización 4 bits
    lcd_write_nibble(lcd, 0x03);
    sleep_ms(5);
    lcd_write_nibble(lcd, 0x03);
    sleep_us(150);
    lcd_write_nibble(lcd, 0x03);
    lcd_write_nibble(lcd, 0x02);

    lcd_send_byte(lcd, 0x28, false); // 4 bits, 2 líneas, 5x8 dots
    lcd_send_byte(lcd, 0x0C, false); // Display ON, cursor OFF
    lcd_send_byte(lcd, 0x06, false); // Entrada automática
    lcd_clear(lcd);
}

void lcd_clear(LCD *lcd) {
    lcd_send_byte(lcd, 0x01, false); // Limpiar pantalla
    sleep_ms(2);
}

void lcd_set_cursor(LCD *lcd, uint8_t row, uint8_t col) {
    uint8_t row_offsets[] = {0x00, 0x40, 0x14, 0x54};
    lcd_send_byte(lcd, 0x80 | (col + row_offsets[row]), false);
}

void lcd_send_char(LCD *lcd, char c) {
    lcd_send_byte(lcd, c, true);
}

void lcd_print(LCD *lcd, const char *str) {
    for (int i = 0; str[i] != '\0'; i++) {
        lcd_send_char(lcd, str[i]);
    }
}
