#ifndef LCD_H
#define LCD_H

#include "pico/stdlib.h"

/**
 * @file lcd.h
 * @brief Librería para manejar pantallas LCD en modo 4 bits con Raspberry Pi Pico.
 */

/**
 * @struct LCD
 * @brief Estructura que define un LCD en modo 4 bits.
 * @var LCD::rs Pin RS (Register Select).
 * @var LCD::en Pin EN (Enable).
 * @var LCD::data Pines de datos D4-D7.
 */
typedef struct {
    uint8_t rs;        /**< Pin RS */
    uint8_t en;        /**< Pin EN */
    uint8_t data[4];   /**< Pines de datos D4-D7 */
} LCD;

/**
 * @brief Inicializa el LCD en modo 4 bits.
 * @param lcd Puntero a la estructura LCD.
 * @param rs Pin RS del LCD.
 * @param en Pin EN del LCD.
 * @param d4 Pin D4 del LCD.
 * @param d5 Pin D5 del LCD.
 * @param d6 Pin D6 del LCD.
 * @param d7 Pin D7 del LCD.
 */
void lcd_init(LCD *lcd, uint8_t rs, uint8_t en, uint8_t d4, uint8_t d5, uint8_t d6, uint8_t d7);

/**
 * @brief Limpia toda la pantalla del LCD y coloca el cursor en la posición inicial.
 * @param lcd Puntero a la estructura LCD.
 */
void lcd_clear(LCD *lcd);

/**
 * @brief Coloca el cursor en una posición específica.
 * @param lcd Puntero a la estructura LCD.
 * @param row Fila (0-indexada).
 * @param col Columna (0-indexada).
 */
void lcd_set_cursor(LCD *lcd, uint8_t row, uint8_t col);

/**
 * @brief Envía un solo carácter al LCD en la posición actual del cursor.
 * @param lcd Puntero a la estructura LCD.
 * @param c Carácter a enviar.
 */
void lcd_send_char(LCD *lcd, char c);

/**
 * @brief Imprime una cadena de texto en el LCD comenzando desde la posición actual del cursor.
 * @param lcd Puntero a la estructura LCD.
 * @param str Cadena de caracteres a imprimir.
 */
void lcd_print(LCD *lcd, const char *str);

#endif
