// LEDController.h  
#pragma once
#ifndef LEDCONTROLLER_H  
#define LEDCONTROLLER_H  
#include <stdint.h>
#include "timex.h"

typedef enum{
    COLOR_NONE,    // (R,G,B): (0,0,0)
    COLOR_RED,     // (R,G,B): (1,0,0)
    COLOR_GREEN,   // (R,G,B): (0,1,0)
    COLOR_YELLO,   // (R,G,B): (1,1,0)
    COLOR_BLUE,    // (R,G,B): (0,0,1)
    COLOR_MAGENTA, // (R,G,B): (1,0,1)
    COLOR_CYAN,    // (R,G,B): (0,1,1)
    COLOR_WHITE,   // (R,G,B): (1,1,1)
}LED_COLOR_t;

class LEDController {  
public:  
    /**
     * LEDController constructor.
     * Initializes the LED controller with the specified GPIO pin.
     *
     * @param gpio The GPIO pin to be used for controlling the LED.
     */
    LEDController(uint8_t gpio_r, uint8_t gpio_g, uint8_t gpio_b);  
    /**
     * Changes the state of the LED to the specified state.
     *
     * @param state The state to set the LED to (0 for OFF, 1 for ON).
     */
    void change_led_color(uint8_t color);

private:  
    uint8_t led_gpio[3];               // led in gpio
    uint8_t rgb[3];                    // 1 | 0
};  
  
#endif  
