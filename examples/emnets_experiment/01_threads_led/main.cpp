#include <cstdio>

#include "clk.h"
#include "board.h"
#include "periph_conf.h"
#include "timex.h"
#include "ztimer.h"
#include "periph/gpio.h"  
#include "thread.h"
#include "msg.h"
#include "shell.h"

#include "ledcontroller.hh"
#define THREAD_STACKSIZE        (THREAD_STACKSIZE_IDLE)

static char stack_for_led_thread[THREAD_STACKSIZE];
static char stack_for_sleep_thread[THREAD_STACKSIZE];

#define LED_MSG_TYPE_ISR     (0x3456)
#define LED_MSG_TYPE_RED     (0x3111)
#define LED_MSG_TYPE_NONE    (0x3110)

#define LED_GPIO_R GPIO26
#define LED_GPIO_G GPIO25
#define LED_GPIO_B GPIO27
// the pid of led thread
static kernel_pid_t _led_pid;
void *_led_thread(void *arg)
{
    (void) arg;
    // init led controller
    LEDController led(LED_GPIO_R, LED_GPIO_G, LED_GPIO_B);
    while(1){
        printf("[LED_THREAD] WAIT\n");
        msg_t msg;
        // Wait for the message from OTHER thread
        msg_receive(&msg);
        if (msg.type == LED_MSG_TYPE_NONE)
        {
            // TURN ON LIGHT
            led.change_led_color(0);
            printf("[LED_THREAD]: LED TURN OFF!!\n");
        }
        else if (msg.type == LED_MSG_TYPE_RED)
        {
            // TURN OFF LIGHT
            led.change_led_color(1);
            printf("[LED_THREAD]: LED TURN ON!!\n");
        }
    }
    return NULL;
}

void *_control_thread(void *arg)
{
    (void) arg;
    uint16_t sleep_ms = 1000;
    uint8_t color = 0;
    while(1){
        // sleep 1000 ms
        ztimer_sleep(ZTIMER_USEC, sleep_ms * US_PER_MS);
        printf("[SLEEP_THREAD]: SLEEP FINISH\n");
        msg_t msg;
        color++;
        if (color % 2 == 1)
        {
            // Tell the led thread to turn on red light
            msg.type = LED_MSG_TYPE_RED;
        }
        else 
        {
            // Tell the led thread to turn off light
            msg.type = LED_MSG_TYPE_NONE;
        }
        // send the message to the led thread(_led_pid)
        if (msg_send(&msg, _led_pid) <= 0){
            printf("[SLEEP_THREAD]: possibly lost interrupt.\n");
        }
        else{
            printf("[SLEEP_THREAD]: Successfully set interrupt.\n");
        }
    }
    return NULL;
}

// static const shell_command_t shell_commands[] = {
//     { NULL, NULL, NULL }
// };

int main(void)
{
    // create led thread
    _led_pid = thread_create(stack_for_led_thread, sizeof(stack_for_led_thread), THREAD_PRIORITY_MAIN - 2,
                            THREAD_CREATE_STACKTEST, _led_thread, NULL,
                            "led");
    if (_led_pid <= KERNEL_PID_UNDEF) {
        printf("[MAIN] Creation of receiver thread failed\n");
        return 1;
    }
    else
    {
        printf("[MAIN] LED_PID: %d\n", _led_pid);
    }
    // create sleep thread
    thread_create(stack_for_sleep_thread, sizeof(stack_for_sleep_thread), THREAD_PRIORITY_MAIN - 1,
                            THREAD_CREATE_STACKTEST, _control_thread, &_led_pid,
                            "sleep");
    printf("[Main] Initialization successful - starting the shell now\n");
    // char line_buf[SHELL_DEFAULT_BUFSIZE];
    // shell_run(shell_commands, line_buf, SHELL_DEFAULT_BUFSIZE);
    while(1);
    return 0;
}
