#pragma once
#include <stdint.h>

// Inicializa LDR en ADC0 (GPIO26)
void     ldr_begin(uint32_t vref_mv, uint16_t samples);

// Lecturas
uint16_t ldr_read_raw(void);         // 0..4095 (promedio)
uint32_t ldr_read_millivolts(void);  // mV = raw * vref / 4095
float    ldr_read_volts(void);       // V  = mV / 1000
