#include "ldr.h"
#include "pico/stdlib.h"
#include "hardware/adc.h"

static uint32_t s_vref_mv = 3300;
static bool     s_inited  = false;

void ldr_begin(uint32_t vref_mv) {
    if (!s_inited) {
        adc_init();
        adc_gpio_init(27);    // GPIO27 -> ADC1
        adc_select_input(1);  // canal 1
        sleep_us(50);         // estabilización MUX
        s_inited = true;
    }
    s_vref_mv = (vref_mv > 0) ? vref_mv : 3300;
}

uint16_t ldr_read_raw(void) {
    adc_select_input(1);  // asegurar canal 1
    (void)adc_read();     // lectura de descarte (opcional)
    return adc_read();    // 12 bits: 0..4095
}

uint32_t ldr_read_millivolts(void) {
    uint16_t raw = ldr_read_raw();
    return (uint32_t)(((uint64_t)raw * (uint64_t)s_vref_mv) / 4095ULL);
}

float ldr_read_volts(void) {
    return (float)ldr_read_millivolts() / 1000.0f;
}