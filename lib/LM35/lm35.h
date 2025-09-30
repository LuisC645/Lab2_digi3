#pragma once
#include <stdint.h>

// Inicializa TODO para LM35 en GPIO26 (ADC0)
void     lm35_begin(uint32_t vref_mv, uint16_t samples);

// Lecturas
uint16_t lm35_read_raw(void);         // 0..4095 (promediado)
uint32_t lm35_read_millivolts(void);  // mV = raw * vref / 4095
float    lm35_read_celsius(void);     // °C = mV / 10
float    lm35_read_volts(void);       // V  = mV / 1000
