#include <stdio.h>
#include "pico/stdlib.h"
#include "lib/LM35/lm35.h"

int main(void) {
    stdio_init_all();

    // Vref=3300 mV, promedio 16 muestras
    lm35_begin(3300, 16);

    while (true) {
        float tC = lm35_read_celsius();
        printf("T = %.2f C\n", tC);

        printf("RAW=%u  V=%lu mV\n", lm35_read_raw(), (unsigned long)lm35_read_millivolts());

        sleep_ms(500);
    }
}
