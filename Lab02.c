#include <stdio.h>
#include <string.h>
#include <math.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"

#include "lib/LDR/ldr.h"
#include "lib/LM35/lm35.h"
#include "lib/lcd/lcd.h"

// -------------------- Config pines (ajusta a tu hardware) --------------------
#define BTN_WIN   14   // Pulsador para N de promedio (8/32/128/512)
#define BTN_FS    15   // Pulsador para fs (500/1000/5000 Hz)

// -------------------- Utilidades LCD ----------------------------------------
static void lcd_print_fixed20(LCD *lcd, int row, const char *text) {
    char buf[21];
    size_t n = strlen(text);
    if (n > 20) n = 20;
    memcpy(buf, text, n);
    for (size_t i = n; i < 20; ++i) buf[i] = ' ';
    buf[20] = '\0';
    lcd_set_cursor(lcd, row, 0);
    lcd_print(lcd, buf);
}

// -------------------- Debounce simple activo-bajo ---------------------------
typedef struct {
    bool last_stable;       // último estado estable (true = alto)
    bool last_sample;       // último leído
    absolute_time_t t_last; // última transición leída
} debounce_t;

static void debounce_init(debounce_t *d, bool initial_level) {
    d->last_stable = initial_level;
    d->last_sample = initial_level;
    d->t_last = get_absolute_time();
}

// Devuelve true si se detectó un "click" (flanco alto->bajo estabilizado)
static bool debounce_update_click(debounce_t *d, bool level_now, uint32_t ms_hold) {
    if (level_now != d->last_sample) {
        d->last_sample = level_now;
        d->t_last = get_absolute_time();
    }
    // Si el estado se mantiene igual por ms_hold, aceptar como estable
    if (absolute_time_diff_us(d->t_last, get_absolute_time()) > (int64_t)ms_hold * 1000) {
        if (level_now != d->last_stable) {
            // cambio estable
            bool prev = d->last_stable;
            d->last_stable = level_now;
            // Click = flanco de 1->0 (activo-bajo)
            if (prev == true && level_now == false) return true;
        }
    }
    return false;
}

// -------------------- Buffers de ventana móvil ------------------------------
#define MAX_WIN 512
static float t_buf[MAX_WIN];  // °C
static float l_buf[MAX_WIN];  // mV
static uint32_t head = 0;     // índice circular
static uint32_t count = 0;    // muestras acumuladas (<= MAX_WIN)
static uint32_t win_size_idx = 0; // índice actual en la lista de tamaños
static const uint16_t WIN_LIST[] = {8, 32, 128, 512};
static const size_t WIN_LIST_N = sizeof(WIN_LIST)/sizeof(WIN_LIST[0]);

static uint16_t current_win(void) { return WIN_LIST[win_size_idx]; }

// Recalcula suma cuando cambia la ventana (toma últimas K muestras)
static void recompute_sums_for_window(float *sumT, float *sumL) {
    *sumT = 0.0f; *sumL = 0.0f;
    uint16_t K = current_win();
    // Tomar min(K, count)
    uint32_t use = count < K ? count : K;
    // Recorrer desde head-1 hacia atrás 'use' muestras
    int32_t idx = (int32_t)head - 1;
    if (idx < 0) idx += MAX_WIN;
    for (uint32_t i = 0; i < use; ++i) {
        *sumT += t_buf[idx];
        *sumL += l_buf[idx];
        idx--; if (idx < 0) idx += MAX_WIN;
    }
}

// Inserta nueva muestra y actualiza sumas de ventana móvil en O(1)
static void window_push(float tC, float l_mV, float *sumT, float *sumL) {
    uint16_t K = current_win();
    // Si buffer aún no lleno, solo sumar e insertar
    if (count < MAX_WIN) {
        t_buf[head] = tC;
        l_buf[head] = l_mV;
        head = (head + 1) % MAX_WIN;
        count++;
    } else {
        // Si está lleno, retiramos el más antiguo (en head) y escribimos encima
        // El más antiguo es el que está en 'head' (porque 'head' apunta a la próxima escritura)
        // Pero para la ventana móvil de tamaño K, solo restamos si ya alcanzamos >= K
        // De todas formas, reemplazar en buffers:
        t_buf[head] = tC;
        l_buf[head] = l_mV;
        head = (head + 1) % MAX_WIN;
    }

    // Actualizar sumas por ventana K
    // Calcular cuántos hay realmente para el promedio actual (<= K)
    uint32_t have = count < K ? count : K;

    if (have < K) {
        // aún no se llenó la ventana
        *sumT += tC;
        *sumL += l_mV;
    } else {
        // ventana llena: restar el elemento que sale y sumar el nuevo
        // El que sale está K posiciones atrás de 'head'
        int32_t idx_out = (int32_t)head - (int32_t)K - 1; // -1 porque ya avanzamos head
        // Ajustar índice circular
        while (idx_out < 0) idx_out += MAX_WIN;
        idx_out %= MAX_WIN;

        *sumT += tC - t_buf[idx_out];
        *sumL += l_mV - l_buf[idx_out];
    }
}

// -------------------- Frecuencias de muestreo -------------------------------
static const uint32_t FS_LIST[] = {500, 1000, 5000};  // Hz
static const size_t FS_LIST_N = sizeof(FS_LIST)/sizeof(FS_LIST[0]);
static size_t fs_idx = 0;

static uint32_t current_fs(void) { return FS_LIST[fs_idx]; }
static int64_t period_us(void)   { return 1000000LL / (int64_t)current_fs(); }

// -------------------- Programa principal ------------------------------------
int main(void) {
    stdio_init_all();
    lm35_begin(3300, 16);
    ldr_begin(3300, 16);

    // Botones activo-bajo con pull-up
    gpio_init(BTN_WIN);
    gpio_set_dir(BTN_WIN, GPIO_IN);
    gpio_pull_up(BTN_WIN);

    gpio_init(BTN_FS);
    gpio_set_dir(BTN_FS, GPIO_IN);
    gpio_pull_up(BTN_FS);

    debounce_t db_win, db_fs;
    debounce_init(&db_win, true); // reposo = alto por pull-up
    debounce_init(&db_fs,  true);

    LCD lcd;
    lcd_init(&lcd, 2, 3, 4, 5, 6, 7); // RS, EN, D4, D5, D6, D7

    // Buffers de líneas (actual/previo) para evitar parpadeo
    char l1[21], l2[21], l3[21], l4[21];
    char p1[21] = "", p2[21] = "", p3[21] = "", p4[21] = "";

    // Estadísticas globales (sesión)
    float t_min = INFINITY, t_max = -INFINITY;
    float l_min = INFINITY, l_max = -INFINITY;

    // Sumas para promedio móvil
    float sumT = 0.0f, sumL = 0.0f;

    // Tiempos
    absolute_time_t t_next_sample = make_timeout_time_us(period_us());
    absolute_time_t t_next_lcd    = make_timeout_time_ms(1000);

    // Bucle
    while (true) {
        // ----------------- Botones con debounce (cada ~5 ms) -----------------
        busy_wait_us_32(5000); // evita loop hiper veloz y ayuda a debounce

        bool click_win = debounce_update_click(&db_win, gpio_get(BTN_WIN), 40);
        if (click_win) {
            // Cambiar ventana: 8 -> 32 -> 128 -> 512 -> 8 ...
            win_size_idx = (win_size_idx + 1) % WIN_LIST_N;
            // Recalcular sumas para nueva ventana
            recompute_sums_for_window(&sumT, &sumL);
        }

        bool click_fs = debounce_update_click(&db_fs, gpio_get(BTN_FS), 40);
        if (click_fs) {
            // Cambiar fs: 500 -> 1000 -> 5000 -> 500 -> ...
            fs_idx = (fs_idx + 1) % FS_LIST_N;
            t_next_sample = make_timeout_time_us(period_us()); // reprogramar periodo
        }

        // ----------------- Muestreo a fs seleccionada ------------------------
        if (time_reached(t_next_sample)) {
            t_next_sample = delayed_by_us(t_next_sample, period_us());

            // Leer sensores
            float tC   = lm35_read_celsius();            // °C
            float l_mV = (float)ldr_read_millivolts();   // mV

            // Actualizar min/max (sesión)
            if (tC   < t_min) t_min = tC;
            if (tC   > t_max) t_max = tC;
            if (l_mV < l_min) l_min = l_mV;
            if (l_mV > l_max) l_max = l_mV;

            // Actualizar ventana móvil
            window_push(tC, l_mV, &sumT, &sumL);
        }

        // ----------------- Refresco LCD cada 1 segundo -----------------------
        if (time_reached(t_next_lcd)) {
            t_next_lcd = delayed_by_ms(t_next_lcd, 1000);

            // Calcular valores a mostrar:
            // Val: última muestra disponible (la más reciente en buffer)
            float t_val = NAN, l_val = NAN;
            if (count > 0) {
                int32_t last_idx = (int32_t)head - 1;
                if (last_idx < 0) last_idx += MAX_WIN;
                t_val = t_buf[last_idx];
                l_val = l_buf[last_idx];
            } else {
                t_val = 0.0f; l_val = 0.0f;
            }

            uint16_t K = current_win();
            uint32_t have = count < K ? count : K;
            float t_avg = (have > 0) ? (sumT / (float)have) : 0.0f;
            float l_avg = (have > 0) ? (sumL / (float)have) : 0.0f;

            // Formato (<=20 chars). Ch1=°C (1 decimal), Ch2=mV (entero):
            // Val:  xx.xC | xxxx
            // Pr:   xx.xC | xxxx
            // Max:  xx.xC | xxxx
            // Min:  xx.xC | xxxx
            snprintf(l1, sizeof(l1), "Val:  %5.1fC | %4.0f", t_val, l_val);
            snprintf(l2, sizeof(l2), "Pr:   %5.1fC | %4.0f", t_avg, l_avg);
            snprintf(l3, sizeof(l3), "Max:  %5.1fC | %4.0f", t_max, l_max);
            snprintf(l4, sizeof(l4), "Min:  %5.1fC | %4.0f", t_min, l_min);

            // Pintar solo si cambió cada línea (sin flicker)
            if (strcmp(l1, p1) != 0) { lcd_print_fixed20(&lcd, 0, l1); strcpy(p1, l1); }
            if (strcmp(l2, p2) != 0) { lcd_print_fixed20(&lcd, 1, l2); strcpy(p2, l2); }
            if (strcmp(l3, p3) != 0) { lcd_print_fixed20(&lcd, 2, l3); strcpy(p3, l3); }
            if (strcmp(l4, p4) != 0) { lcd_print_fixed20(&lcd, 3, l4); strcpy(p4, l4); }

            // (Opcional) Log de estado
            printf("[fs=%lu Hz, N=%u, have=%u]\n",
                   (unsigned long)current_fs(), current_win(), have);
        }
    }
}
