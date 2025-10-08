#include <stdio.h>
#include <string.h>
#include <math.h>
#include "pico/stdlib.h"

#include "lib/LM35/lm35.h"     // lm35_begin(uint32_t vref_mv)
#include "lib/LDR/ldr.h"       // ldr_begin(uint32_t vref_mv)
#include "lib/lcd/lcd.h"       // lcd_init, lcd_set_cursor, lcd_print
#include "lib/util/util.h"     // botones fs + N

/* Helper: 20 columnas exactas */
static void lcd_print_fixed20(LCD *lcd, uint8_t row, const char *text) {
    char buf[21];
    size_t n = 0;
    while (text[n] && n < 20) buf[n++] = text[n];
    while (n < 20)            buf[n++] = ' ';
    buf[20] = '\0';
    lcd_set_cursor(lcd, row, 0);
    lcd_print(lcd, buf);
}

/* Historial para promedio (guardamos mucho por si acaso) */
#define MAX_HIST 5000
static float    t_hist[MAX_HIST];
static uint32_t l_hist[MAX_HIST];
static uint32_t hist_head  = 0;
static uint32_t hist_count = 0;

static inline void hist_push(float tC, uint32_t l_mV) {
    t_hist[hist_head] = tC;
    l_hist[hist_head] = l_mV;
    hist_head = (hist_head + 1) % MAX_HIST;
    if (hist_count < MAX_HIST) hist_count++;
}

static void hist_avg_last_M(uint32_t M, float *t_avg, float *l_avg) {
    if (M == 0 || hist_count == 0) { *t_avg = 0.0f; *l_avg = 0.0f; return; }
    if (M > hist_count) M = hist_count;

    int32_t idx = (int32_t)hist_head - 1;
    if (idx < 0) idx += MAX_HIST;

    double sumT = 0.0, sumL = 0.0;
    for (uint32_t i = 0; i < M; ++i) {
        sumT += t_hist[idx];
        sumL += l_hist[idx];
        idx--; if (idx < 0) idx += MAX_HIST;
    }
    *t_avg = (float)(sumT / (double)M);
    *l_avg = (float)(sumL / (double)M);
}

/* ------------------- Programa principal ------------------- */
int main(void) {
    stdio_init_all();

    utils_buttons_begin();   // botones fs + N

    lm35_begin(3300);
    ldr_begin(3300);

    LCD lcd;
    lcd_init(&lcd, 2, 3, 4, 5, 6, 7); // RS, EN, D4..D7

    char l1[21] = "", l2[21] = "", l3[21] = "", l4[21] = "";

    float    t_min = INFINITY, t_max = -INFINITY;
    uint32_t l_min = UINT32_MAX, l_max = 0;

    float    t_val = 0.0f;
    uint32_t l_val = 0;

    absolute_time_t t_next_sample = make_timeout_time_us(utils_period_us());
    absolute_time_t t_next_lcd    = make_timeout_time_ms(1000);

    /* ---- Banner inicial ---- */
    printf("=== DataLogger LM35/LDR ===\n");
    printf("fs inicial: %lu Hz\n", (unsigned long)utils_get_fs());
    printf("N inicial:  %u muestras\n", (unsigned)utils_get_N());
    printf("BTN_FS=GPIO15, BTN_MEDIA=GPIO14 (activo-bajo)\n");

    while (true) {
        /* Botones */
        sleep_ms(5);
        utils_buttons_task();

        if (utils_consume_fs_changed()) {
            t_next_sample = make_timeout_time_us(utils_period_us());
            uint32_t fs_now = utils_get_fs();
            printf("[BTN] fs -> %lu Hz\n", (unsigned long)fs_now);
        }
        if (utils_consume_n_changed()) {
            uint16_t N_now = utils_get_N();
            printf("[BTN] N  -> %u muestras\n", (unsigned)N_now);
            /* No hace falta tocar el historial; el promedio usa N nuevo en el siguiente refresco */
        }

        /* Muestreo a fs actual */
        if (time_reached(t_next_sample)) {
            t_next_sample = delayed_by_us(t_next_sample, utils_period_us());

            t_val = lm35_read_celsius();        // °C
            l_val = ldr_read_millivolts();      // mV

            if (t_val < t_min) t_min = t_val;
            if (t_val > t_max) t_max = t_val;
            if (l_val < l_min) l_min = l_val;
            if (l_val > l_max) l_max = l_val;

            hist_push(t_val, l_val);
        }

        /* Refresco LCD cada 1 s */
        if (time_reached(t_next_lcd)) {
            t_next_lcd = delayed_by_ms(t_next_lcd, 1000);

            /* Promedio sobre las últimas N muestras */
            uint32_t M = utils_get_N();
            float t_avg = 0.0f, l_avg = 0.0f;
            hist_avg_last_M(M, &t_avg, &l_avg);

            char n1[21], n2[21], n3[21], n4[21];

            /* 1: actuales */
            snprintf(n1, sizeof(n1), "Val %5.1fC | %4lu",
                     t_val, (unsigned long)l_val);

            /* 2: promedio */
            snprintf(n2, sizeof(n2), "Pro %5.1fC | %4lu",
                     t_avg, (unsigned long)l_avg);

            /* 3: máximos */
            snprintf(n3, sizeof(n3), "Max %5.1fC | %4lu",
                     t_max, (unsigned long)l_max);

            /* 4: mínimos */
            snprintf(n4, sizeof(n4), "Min %5.1fC | %4lu",
                     t_min, (unsigned long)l_min);

            if (strcmp(n1, l1) != 0) { lcd_print_fixed20(&lcd, 0, n1); strcpy(l1, n1); }
            if (strcmp(n2, l2) != 0) { lcd_print_fixed20(&lcd, 1, n2); strcpy(l2, n2); }
            if (strcmp(n3, l3) != 0) { lcd_print_fixed20(&lcd, 2, n3); strcpy(l3, n3); }
            if (strcmp(n4, l4) != 0) { lcd_print_fixed20(&lcd, 3, n4); strcpy(l4, n4); }
        }
    }
}
