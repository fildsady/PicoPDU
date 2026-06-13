#include "i2c_lcd.h"

static int addr = 0x27; // default PCF8574 I2C address

char line0_buff[17];
char line1_buff[17];

void i2c_write_byte(uint8_t val) {
    i2c_write_blocking(I2C_MODULE, addr, &val, 1, 0);
}

void lcd_toggle_enable(uint8_t val) {
#define DELAY_US 400
    sleep_us(DELAY_US);
    i2c_write_byte(val | LCD_ENABLE_BIT);
    sleep_us(DELAY_US);
    i2c_write_byte(val & ~LCD_ENABLE_BIT);
    sleep_us(DELAY_US);
}

void lcd_send_byte(uint8_t val, int mode) {
    uint8_t high = mode | (val & 0xF0) | LCD_BACKLIGHT;
    uint8_t low  = mode | ((val << 4) & 0xF0) | LCD_BACKLIGHT;
    i2c_write_byte(high);
    lcd_toggle_enable(high);
    i2c_write_byte(low);
    lcd_toggle_enable(low);
}

void lcd_clear(void) {
    lcd_send_byte(LCD_CLEARDISPLAY, LCD_COMMAND);
}

void lcd_set_cursor(int line, int position) {
    int val = (line == 0) ? 0x80 + position : 0xC0 + position;
    lcd_send_byte(val, LCD_COMMAND);
}

static inline void lcd_char(char val) {
    lcd_send_byte(val, LCD_CHARACTER);
}

void lcd_string(const char *s) {
    while (*s) lcd_char(*s++);
}

void lcd_wireLine(const char *s) {
    int i = 16;
    while (i--) {
        if (*s == '\0') lcd_char(0x20);
        else            lcd_char(*s++);
    }
}

void lcd_clear_screen(void) {
    lcd_set_cursor(0, 0);
    int i = 32;
    while (i--) lcd_char(0x20);
}

void lcd_clear_buff_all(void) {
    memset(line0_buff, 0x20, sizeof(line0_buff));
    memset(line1_buff, 0x20, sizeof(line1_buff));
}

void lcd_clear_buff_line(uint8_t line) {
    if (line == 0) memset(line0_buff, 0x20, sizeof(line0_buff));
    if (line == 1) memset(line1_buff, 0x20, sizeof(line1_buff));
}

void put_buff_to_lcd(void) {
    lcd_set_cursor(0, 0);
    lcd_wireLine(line0_buff);
    lcd_set_cursor(1, 0);
    lcd_wireLine(line1_buff);
}

int lcd_buff_printf(int line, int position, const char *format, ...) {
    va_list ap;
    va_start(ap, format);
    size_t len = 0;
    if (line == 0) len = vsnprintf(&line0_buff[position], sizeof(line0_buff) - position, format, ap);
    if (line == 1) len = vsnprintf(&line1_buff[position], sizeof(line1_buff) - position, format, ap);
    va_end(ap);
    return len;
}

int lcd_printf(int line, int position, const char *format, ...) {
    char buf[32];
    va_list ap;
    va_start(ap, format);
    size_t len = vsnprintf(buf, sizeof(buf), format, ap);
    lcd_set_cursor(line, position);
    lcd_string(buf);
    va_end(ap);
    return len;
}

void lcd_init(void) {
    i2c_init(I2C_MODULE, 125 * 1000);
    gpio_set_function(SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(SDA_PIN);
    gpio_pull_up(SCL_PIN);
    bi_decl(bi_2pins_with_func(SDA_PIN, SCL_PIN, GPIO_FUNC_I2C));

    lcd_send_byte(0x03, LCD_COMMAND);
    lcd_send_byte(0x03, LCD_COMMAND);
    lcd_send_byte(0x03, LCD_COMMAND);
    lcd_send_byte(0x02, LCD_COMMAND);
    lcd_send_byte(LCD_ENTRYMODESET | LCD_ENTRYLEFT, LCD_COMMAND);
    lcd_send_byte(LCD_FUNCTIONSET  | LCD_2LINE,     LCD_COMMAND);
    lcd_send_byte(LCD_DISPLAYCONTROL | LCD_DISPLAYON, LCD_COMMAND);
    lcd_clear();
}
