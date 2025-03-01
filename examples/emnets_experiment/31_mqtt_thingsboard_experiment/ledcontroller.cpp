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
    // Assign GPIO pins to class members
    led_gpio[0] = gpio_r;  // Red pin
    led_gpio[1] = gpio_g;  // Green pin
    led_gpio[2] = gpio_b;  // Blue pin
    
    // Initialize the GPIO pins as output mode
    gpio_init(led_gpio[0], GPIO_OUT);  // Red pin
    gpio_init(led_gpio[1], GPIO_OUT);  // Green pin
    gpio_init(led_gpio[2], GPIO_OUT);  // Blue pin

    // Optional: Store the initial RGB state as off
    rgb[0] = 0;  // Red state
    rgb[1] = 0;  // Green state
    rgb[2] = 0;  // Blue state
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
    switch(color) {
        case COLOR_RED:
            gpio_write(led_gpio[0], 1);  // Red ON
            gpio_write(led_gpio[1], 0);  // Green OFF
            gpio_write(led_gpio[2], 0);  // Blue OFF
            rgb[0] = 1; rgb[1] = 0; rgb[2] = 0;
            break;
        case COLOR_GREEN:
            gpio_write(led_gpio[0], 0);  // Red OFF
            gpio_write(led_gpio[1], 1);  // Green ON
            gpio_write(led_gpio[2], 0);  // Blue OFF
            rgb[0] = 0; rgb[1] = 1; rgb[2] = 0;
            break;
        case COLOR_BLUE:
            gpio_write(led_gpio[0], 0);  // Red OFF
            gpio_write(led_gpio[1], 0);  // Green OFF
            gpio_write(led_gpio[2], 1);  // Blue ON
            rgb[0] = 0; rgb[1] = 0; rgb[2] = 1;
            break;
        case COLOR_YELLOW:
            gpio_write(led_gpio[0], 1);  // Red ON
            gpio_write(led_gpio[1], 1);  // Green ON
            gpio_write(led_gpio[2], 0);  // Blue OFF
            rgb[0] = 1; rgb[1] = 1; rgb[2] = 0;
            break;
        case COLOR_WHITE:
            gpio_write(led_gpio[0], 1);  // Red ON
            gpio_write(led_gpio[1], 1);  // Green ON
            gpio_write(led_gpio[2], 1);  // Blue ON
            rgb[0] = 1; rgb[1] = 1; rgb[2] = 1;
            break;
        case COLOR_MAGENTA:
            gpio_write(led_gpio[0], 1);  // Red ON
            gpio_write(led_gpio[1], 0);  // Green OFF
            gpio_write(led_gpio[2], 1);  // Blue ON
            rgb[0] = 1; rgb[1] = 0; rgb[2] = 1;
            break;
        case COLOR_CYAN:
            gpio_write(led_gpio[0], 0);  // Red OFF
            gpio_write(led_gpio[1], 1);  // Green ON
            gpio_write(led_gpio[2], 1);  // Blue ON
            rgb[0] = 0; rgb[1] = 1; rgb[2] = 1;
            break;
        case COLOR_NONE:
        default:
            gpio_write(led_gpio[0], 0);  // Red OFF
            gpio_write(led_gpio[1], 0);  // Green OFF
            gpio_write(led_gpio[2], 0);  // Blue OFF
            rgb[0] = 0; rgb[1] = 0; rgb[2] = 0;
            break;
    }
}
// Getter for the Red LED value
uint8_t LEDController::get_r_value() const {
    return rgb[0];  // Return the state of the Red channel
}

// Getter for the Green LED value
uint8_t LEDController::get_g_value() const {
    return rgb[1];  // Return the state of the Green channel
}

// Getter for the Blue LED value
uint8_t LEDController::get_b_value() const {
    return rgb[2];  // Return the state of the Blue channel
}

