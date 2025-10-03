#include <stdio.h>
#include "pico/stdlib.h"
#include "lib/LDR/ldr.h"
#include "lib/lcd/lcd.h"

int main(void) {
    stdio_init_all();
    LCD lcd;
    lcd_init(&lcd, 2, 3, 4, 5, 6, 7); // RS, EN, D4, D5, D6, D7
    lcd_print(&lcd, "Hola Mundo!");
    while (true) {

        sleep_ms(500);  
    }
}
