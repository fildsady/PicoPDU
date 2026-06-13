#ifndef I2C_LCD_H
#define I2C_LCD_H

#include "hardware/i2c.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <math.h>
#include "pico/binary_info.h"
#include "pico/stdlib.h"

#define I2C_MODULE  i2c1
#define SDA_PIN     2   // GP2 → LCD SDA
#define SCL_PIN     3   // GP3 → LCD SCL

#define LCD_CLEARDISPLAY    0x01
#define LCD_RETURNHOME      0x02
#define LCD_ENTRYMODESET    0x04
#define LCD_DISPLAYCONTROL  0x08
#define LCD_CURSORSHIFT     0x10
#define LCD_FUNCTIONSET     0x20
#define LCD_SETCGRAMADDR    0x40
#define LCD_SETDDRAMADDR    0x80

#define LCD_ENTRYSHIFTINCREMENT 0x01
#define LCD_ENTRYLEFT           0x02

#define LCD_BLINKON     0x01
#define LCD_CURSORON    0x02
#define LCD_DISPLAYON   0x04

#define LCD_MOVERIGHT   0x04
#define LCD_DISPLAYMOVE 0x08

#define LCD_5x10DOTS 0x04
#define LCD_2LINE    0x08
#define LCD_8BITMODE 0x10

#define LCD_BACKLIGHT   0x08
#define LCD_ENABLE_BIT  0x04

#define LCD_CHARACTER  1
#define LCD_COMMAND    0

#define MAX_LINES  2
#define MAX_CHARS  16

extern char line0_buff[17];
extern char line1_buff[17];

void i2c_write_byte(uint8_t val);
void lcd_toggle_enable(uint8_t val);
void lcd_send_byte(uint8_t val, int mode);
void lcd_wireLine(const char *s);

void lcd_init(void);
void lcd_clear(void);
void lcd_clear_screen(void);

void lcd_clear_buff_all(void);
void lcd_clear_buff_line(uint8_t line);
int  lcd_buff_printf(int line, int position, const char *format, ...);
void put_buff_to_lcd(void);

void lcd_set_cursor(int line, int position);
void lcd_string(const char *s);
int  lcd_printf(int line, int position, const char *format, ...);

#endif
