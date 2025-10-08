#include "util.h"
#include "pico/stdlib.h"
#include "hardware/gpio.h"

/* Opciones fijas */
static const uint32_t FS_OPTS[] = { 500, 1000, 5000 };
static const uint16_t N_OPTS[]  = { 8, 32, 128, 512 };

/* Estado interno */
static uint8_t s_fs_idx      = 1;    /* fs=1000 Hz */
static uint8_t s_n_idx       = 0;    /* N = 8 */
static bool    s_fs_last     = true; /* último nivel leído */
static bool    s_n_last      = true;
static bool    s_flag_fs_chg = false;
static bool    s_flag_n_chg  = false;

/* Getters */
uint32_t utils_get_fs(void)    { return FS_OPTS[s_fs_idx]; }
uint16_t utils_get_N(void)     { return N_OPTS[s_n_idx]; }
int64_t  utils_period_us(void) { return 1000000LL / (int64_t)FS_OPTS[s_fs_idx]; }

/* One-shots */
bool utils_consume_fs_changed(void) {
    bool f = s_flag_fs_chg; s_flag_fs_chg = false; return f;
}
bool utils_consume_n_changed(void) {
    bool f = s_flag_n_chg;  s_flag_n_chg  = false; return f;
}

void utils_buttons_begin(void) {
    /* FS */
    gpio_init(BTN_FS);    gpio_set_dir(BTN_FS, GPIO_IN);    gpio_pull_up(BTN_FS);
    s_fs_last = gpio_get(BTN_FS);
    s_flag_fs_chg = false;

    /* N (MEDIA) */
    gpio_init(BTN_MEDIA); gpio_set_dir(BTN_MEDIA, GPIO_IN); gpio_pull_up(BTN_MEDIA);
    s_n_last = gpio_get(BTN_MEDIA);
    s_flag_n_chg = false;
}

void utils_buttons_task(void) {
    /* FS */
    bool fs_now = gpio_get(BTN_FS);
    if (s_fs_last && !fs_now) {
        s_fs_idx = (uint8_t)((s_fs_idx + 1u) % 3u);
        s_flag_fs_chg = true;
    }
    s_fs_last = fs_now;

    /* N */
    bool n_now = gpio_get(BTN_MEDIA);
    if (s_n_last && !n_now) {
        s_n_idx = (uint8_t)((s_n_idx + 1u) % 4u);
        s_flag_n_chg = true;
    }
    s_n_last = n_now;
}
