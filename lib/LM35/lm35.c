#include "lm35.h"
#include "pico/stdlib.h"
#include "hardware/adc.h"

static uint32_t s_vref_mv  = 3300;
static bool     s_inited   = false;

void lm35_begin(uint32_t vref_mv) {
    if (!s_inited) {
        adc_init();            // periférico ADC
        adc_gpio_init(26);     // GPIO26 -> ADC0
        adc_select_input(0);   // canal 0
        sleep_us(50);          // estabilización MUX
        s_inited = true;
    }
    s_vref_mv = (vref_mv > 0) ? vref_mv : 3300;
}

/* Una sola conversión (sin promediar). */
uint16_t lm35_read_raw(void) {
    adc_select_input(0);    // asegurar canal
    (void)adc_read();       // lectura de descarte (opcional)
    return adc_read();      // 12 bits: 0..4095
}

uint32_t lm35_read_millivolts(void) {
    uint16_t raw = lm35_read_raw();
    /* mV = raw / 4095 * vref_mv */
    return (uint32_t)(( (uint64_t)raw * (uint64_t)s_vref_mv ) / 4095ULL);
}

float lm35_read_celsius(void) {
    /* LM35: 10 mV/°C */
    return (float)lm35_read_millivolts() / 10.0f;
}

float lm35_read_volts(void) {
    return (float)lm35_read_millivolts() / 1000.0f;
}
