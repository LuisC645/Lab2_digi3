#ifndef LM35_H
#define LM35_H

#include <stdint.h>

/* Inicializa el ADC para el LM35 en ADC0 (GPIO26).
   vref_mv: tensión de referencia en mV (ej. 3300). */
void lm35_begin(uint32_t vref_mv);

/* Lecturas básicas */
uint16_t lm35_read_raw(void);          /* 0..4095 (12 bits) */
uint32_t lm35_read_millivolts(void);   /* mV */
float    lm35_read_celsius(void);      /* °C (10 mV/°C) */
float    lm35_read_volts(void);        /* V */

#endif /* LM35_H */
