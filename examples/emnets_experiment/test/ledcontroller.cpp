#include "ztimer.h"
#include "ledcontroller.hh"
#include "periph/gpio.h"  
#include <stdio.h>
/**
 * Initialize the RGB pins as output mode, and initially, the lights should be off.
 * gpio_init(gpio_pin, GPIO_OUT);
 * gpio_write(pin, 0);
 */
LEDController::LEDController(uint8_t gpio_r, uint8_t gpio_g, uint8_t gpio_b){  
    printf("LED Controller initialized with (RGB: GPIO%d, GPIO%d, GPIO%d)\n", gpio_r, gpio_g, gpio_b);
    // input your code
    led_gpio[0] = gpio_r;
    led_gpio[1] = gpio_g;
    led_gpio[2] = gpio_b;
    for (int i = 0; i < 3; ++i)
    {
        rgb[i] = 0;
        gpio_init(led_gpio[i], GPIO_OUT);
        gpio_write(led_gpio[i], rgb[i]);
    }
}  

/**
 * Implement a light that displays at least 5 status colors through the RGB three pins.
 * ------------------------------------------------------------
 * @note Method 1
 * Utilizes the gpio_write function to set the GPIO pin connected to the LED to the current LED state.
 * void gpio_write(uint8_t pin, int value);
 * @param pin The GPIO pin connected to the LED.
 * @param value The value to set the GPIO pin to (0 for LOW, 1 for HIGH).
 * ------------------------------------------------------------
 * @note Method 2
 * Uses the gpio_set and gpio_clear functions to set the GPIO pin connected to the LED to the current LED state.
 * void gpio_set(uint8_t pin); void gpio_clear(uint8_t pin);
 * @param pin The GPIO pin connected to the LED.
 */
void LEDController::change_led_color(uint8_t color){  
    // input your code
    rgb[0] = (color >> 2) & 0x1;
    rgb[1] = (color >> 1) & 0x1;
    rgb[2] = (color & 0x1);
    for (int i = 0; i < 3; ++i)
    {
        gpio_write(led_gpio[i], rgb[i]);
    }
}
