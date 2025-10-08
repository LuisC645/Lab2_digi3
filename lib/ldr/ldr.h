#ifndef LDR_H
#define LDR_H

#include <stdint.h>

/* Inicializa el ADC para el LDR en ADC1 (GPIO27).
   vref_mv: tensión de referencia en mV (ej. 3300). */
void ldr_begin(uint32_t vref_mv);

/* Lecturas básicas (una sola conversión, sin promediar) */
uint16_t ldr_read_raw(void);         /* 0..4095 (12 bits) */
uint32_t ldr_read_millivolts(void);  /* mV = raw/4095 * vref_mv */
float    ldr_read_volts(void);       /* V  = mV / 1000 */

#endif /* LDR_H */
