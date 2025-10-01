#include "ldr.h"
#include "pico/stdlib.h"
#include "hardware/adc.h"

static bool     s_inited   = false;
static uint32_t s_vref_mv  = 3300;
static uint16_t s_samples  = 16;

void ldr_begin(uint32_t vref_mv, uint16_t samples) {
    if (!s_inited) {
        adc_init();
        adc_gpio_init(27);      // GPIO26 -> ADC0
        adc_select_input(0);
        s_inited = true;
        sleep_us(50);
    }
    s_vref_mv = (vref_mv > 0) ? vref_mv : 3300;
    s_samples = (samples > 0) ? samples : 8;
}

static inline uint16_t read_raw_avg(uint16_t n) {
    if (n == 0) n = 1;
    adc_select_input(0);
    (void)adc_read();           // descarta 1 lectura
    uint32_t acc = 0;
    for (uint16_t i = 0; i < n; ++i) {
        acc += adc_read();      // 12 bits: 0..4095
    }
    return (uint16_t)(acc / n);
}

uint16_t ldr_read_raw(void) {
    return read_raw_avg(s_samples);
}

uint32_t ldr_read_millivolts(void) {
    uint16_t raw = ldr_read_raw();
    return (uint32_t)(( (uint64_t)raw * (uint64_t)s_vref_mv ) / 4095ULL);
}

float ldr_read_volts(void) {
    return (float)ldr_read_millivolts() / 1000.0f;
}
