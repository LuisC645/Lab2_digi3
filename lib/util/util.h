#ifndef UTILS_H
#define UTILS_H

#include <stdint.h>
#include <stdbool.h>

/* Pines (activo-bajo con pull-up). Puedes redefinirlos antes de incluir. */
#ifndef BTN_MEDIA
#define BTN_MEDIA   14   /* Cambia N: 8/32/128/512 */
#endif
#ifndef BTN_FS
#define BTN_FS      15   /* Cambia fs: 500/1000/5000 Hz */
#endif

/* Inicializa GPIO y estado interno de botones. */
void utils_buttons_begin(void);

/* Llamar en el loop (p.ej. cada 1–5 ms). Detecta flanco 1->0 simple. */
void utils_buttons_task(void);

/* “One-shot”: true si cambió fs/N desde la última vez (y limpia la bandera). */
bool     utils_consume_fs_changed(void);
bool     utils_consume_n_changed(void);

/* Parámetros actuales y periodo */
uint32_t utils_get_fs(void);   /* {500,1000,5000} Hz */
uint16_t utils_get_N(void);    /* {8,32,128,512}   */
int64_t  utils_period_us(void);/* 1e6 / fs */

#endif /* UTILS_H */
