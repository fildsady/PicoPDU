#include "pico/stdlib.h"

// ---------------------------------------------------------------------------
// LED abstraction — รองรับทั้ง Pico 2 (GPIO25) และ Pico 2W (CYW43)
// ---------------------------------------------------------------------------
#ifdef CYW43_WL_GPIO_LED_PIN
#include "pico/cyw43_arch.h"
static void led_init(void)   { cyw43_arch_init(); }
static void led_set(bool on) { cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, on); }
#else
#ifndef PICO_DEFAULT_LED_PIN
#define PICO_DEFAULT_LED_PIN 25   // Pico 2 onboard LED
#endif
static void led_init(void) {
    gpio_init(PICO_DEFAULT_LED_PIN);
    gpio_set_dir(PICO_DEFAULT_LED_PIN, GPIO_OUT);
}
static void led_set(bool on) { gpio_put(PICO_DEFAULT_LED_PIN, on); }
#endif

#define BLINK_MS 500

int main(void) {
    stdio_init_all();
    led_init();

    while (true) {
        led_set(true);
        sleep_ms(BLINK_MS);
        led_set(false);
        sleep_ms(BLINK_MS);
    }
}
