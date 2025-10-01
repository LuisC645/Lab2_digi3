#include <stdio.h>
#include "pico/stdlib.h"
#include "lib/LDR/ldr.h"

int main(void) {
    stdio_init_all();
    sleep_ms(1500);

    ldr_begin(3300, 16); // Vref=3300 mV, promedio 16

    while (true) {
        printf("LDR: raw=%u  V=%lu mV\n",
               ldr_read_raw(),
               (unsigned long)ldr_read_millivolts());
        sleep_ms(500);
    }
}
