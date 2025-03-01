#include <stdio.h>
#include <stdlib.h>
#include <cmath>
#include <string>
#include <log.h>
#include <errno.h>
#include "clk.h"
#include "board.h"
#include "periph_conf.h"
#include "timex.h"
#include "ztimer.h"
#include "periph/gpio.h"  
#include "thread.h"
#include "msg.h"
#include "shell.h"
// #include "xtimer.h"
#include "ledcontroller.hh"
#include "mpu6050.h"
#define THREAD_STACKSIZE        (THREAD_STACKSIZE_IDLE)
static char stack_for_led_thread[THREAD_STACKSIZE];
static char stack_for_imu_thread[THREAD_STACKSIZE];
// static char stack_for_sleep_thread[THREAD_STACKSIZE];

static kernel_pid_t _led_pid;
#define LED_MSG_TYPE_ISR     (0x3456)
#define LED_MSG_TYPE_NONE    (0x3110)
#define LED_MSG_TYPE_RED     (0x3111)
#define LED_MSG_TYPE_GREEN   (0x3112)
#define LED_MSG_TYPE_BLUE    (0x3113)
#define LED_MSG_TYPE_YELLOW  (0x3114)
#define LED_MSG_TYPE_WHITE   (0x3115)
#define LED_MSG_TYPE_MAGENTA (0x3116)
#define LED_MSG_TYPE_CYAN    (0x3117)

#define LED_GPIO_R GPIO26
#define LED_GPIO_G GPIO25
#define LED_GPIO_B GPIO27
//(0.03, -0.03, 9.52) (m/s²), (-7.33, 1.55, -3.08) (°/s)
#define calibration_data_ax (0.03)
#define calibration_data_ay (-0.03)
#define calibration_data_az (9.52 - g_acc)
#define calibration_data_gx (-7.33)
#define calibration_data_gy (1.55)
#define calibration_data_gz (-3.08)

struct MPU6050Data
{
    float ax, ay, az;
    float gx, gy, gz;
};
enum MoveState{Stationary_h, Stationary_v, Tilted, Rotating, Moving_h, Moving_v};

void delay_ms(uint32_t sleep_ms)
{
    ztimer_sleep(ZTIMER_USEC, sleep_ms * US_PER_MS);
    return;
}
/**
 * LED control thread function.
 * Then, it enters an infinite loop where it waits for messages to control the LED.
 * @param arg Unused argument.
 * @return NULL.
 */
void *_led_thread(void *arg)
{
    (void) arg;
    LEDController led(LED_GPIO_R, LED_GPIO_G, LED_GPIO_B);
    led.change_led_color(0);
    while(1){
        // Input your codes
        // Wait for a message to control the LED
        // Display different light colors based on the motion state of the device.
        printf("[LED_THREAD] WAIT\n");
        msg_t msg;
        // Wait for the message from the sleep thread
        msg_receive(&msg);
        switch (msg.type) {
            case LED_MSG_TYPE_NONE:
                led.change_led_color(COLOR_NONE);// 0
                printf("[LED_THREAD]: LED TURN OFF!!\n");
                break;
            case LED_MSG_TYPE_RED:
                led.change_led_color(COLOR_RED);// 1
                printf("[LED_THREAD]: LED RED ON!!\n");
                break;
            case LED_MSG_TYPE_GREEN:
                led.change_led_color(COLOR_GREEN);// 2
                printf("[LED_THREAD]: LED GREEN ON!!\n");
                break;
            case LED_MSG_TYPE_YELLOW:
                led.change_led_color(COLOR_YELLOW);// 3
                printf("[LED_THREAD]: LED YELLOW ON!!\n");
                break;
            case LED_MSG_TYPE_BLUE:
                led.change_led_color(COLOR_BLUE);// 4
                printf("[LED_THREAD]: LED BLUE ON!!\n");
                break;
            case LED_MSG_TYPE_MAGENTA:
                led.change_led_color(COLOR_MAGENTA);// 5
                printf("[LED_THREAD]: LED MAGENTA ON!!\n");
                break;
            case LED_MSG_TYPE_CYAN:
                led.change_led_color(COLOR_CYAN);// 6
                printf("[LED_THREAD]: LED CYAN ON!!\n");
                break;
            case LED_MSG_TYPE_WHITE:
                led.change_led_color(COLOR_WHITE);// 7
                printf("[LED_THREAD]: LED WHITE ON!!\n");
                break;
            default:
                led.change_led_color(COLOR_NONE);
                printf("[LED_THREAD]: UNKNOWN MESSAGE!!\n");
                break;
        }

    }
    return NULL;
}

// void *_control_thread(void *arg)
// {
//     (void) arg;
//     uint16_t sleep_ms = 1000;
//     uint8_t color = 0;
//     while(1){
//         // sleep 1000 ms
//         ztimer_sleep(ZTIMER_USEC, sleep_ms * US_PER_MS);
//         printf("[CONTROL_THREAD]: RUN SMOOTHLY\n");
//         msg_t msg;
        
//         // Cycle through colors: green, red, blue, yellow, white, and off
//         switch (color % 8) {
//             case 0:
//                 msg.type = LED_MSG_TYPE_NONE; 
//                 break;
//             case 1:
//                 msg.type = LED_MSG_TYPE_RED; 
//                 break;
//             case 2:
//                 msg.type = LED_MSG_TYPE_GREEN; 
//                 break;
//             case 3:
//                 msg.type = LED_MSG_TYPE_BLUE;
//                 break;
//             case 4:
//                 msg.type = LED_MSG_TYPE_YELLOW;
//                 break;
//             case 5:
//                 msg.type = LED_MSG_TYPE_WHITE; 
//                 break;
//             case 6:
//                 msg.type = LED_MSG_TYPE_MAGENTA; 
//                 break;
//             case 7:
//                 msg.type = LED_MSG_TYPE_CYAN; 
//                 break;
//             default:
//                 msg.type = LED_MSG_TYPE_NONE;
//                 break;
//         }
        
//         // Send the message to the led thread (_led_pid)
//         if (msg_send(&msg, _led_pid) <= 0){
//             printf("[SLEEP_THREAD]: possibly lost interrupt.\n");
//         }
//         else{
//             printf("[SLEEP_THREAD]: Successfully set interrupt.\n");
//         }
        
//         color++;  // Increment the color index
//     }
//     return NULL;
// }

#define g_acc (9.8)
/*
Input your code
Please use your imagination or search online 
to determine the motion state of the device 
based on the data obtained from the MPU6050 sensor.
*/


MoveState detectMovement(MPU6050Data &data)
{
    // Thresholds for detecting movement
    const float gyro_threshold = 20.0;  // Adjust this threshold as needed for sensitivity
    const float acc_threshold = 1.0;   // Acceleration threshold for moving
    const float tilt_threshold = 6.0;  // Tilt threshold for detecting tilted stillness

    // Calculate the total acceleration magnitude
    float acc_magnitude = sqrt(data.ax * data.ax + data.ay * data.ay + data.az * data.az);
    //float gyro_magnitude = sqrt(data.gx*data.gx + data.gy*data.gy + data.gz*data.gz);

    // Check if the device is stationary (horizontal stillness)
    if (fabs(fabs(data.az)-g_acc) < acc_threshold && fabs(data.ax)<acc_threshold && fabs(data.ay)<acc_threshold && 
        fabs(data.gx) < gyro_threshold && fabs(data.gy) < gyro_threshold && fabs(data.gz) < gyro_threshold) {
        return Stationary_h; // YELLOW LIGHT
    }

    //Check if the device is stationary (vertical stillness)
    if (
        ((fabs(fabs(data.ay)-g_acc) < acc_threshold && fabs(data.ax)<acc_threshold && fabs(data.az)<acc_threshold) ||
        (fabs(fabs(data.ax)-g_acc) < acc_threshold && fabs(data.ay)<acc_threshold && fabs(data.az)<acc_threshold))&& 
        fabs(data.gx) < gyro_threshold && fabs(data.gy) < gyro_threshold && fabs(data.gz) < gyro_threshold) {
        return Stationary_v; //MAGETA LIGHT
    }

    //Check if the device is moving vertically
    if(
        ((fabs(data.gx) < gyro_threshold) + (fabs(data.gy) < gyro_threshold) + (fabs(data.gz) < gyro_threshold))>=2
         && fabs(data.ax)<acc_threshold && fabs(data.ay)<acc_threshold && fabs(data.az)>acc_threshold){
        return Moving_v; // CYAN LIGHT
    }

    // Check if the device is tilted but not moving (tilted stillness)
    if (acc_magnitude > tilt_threshold && 
        fabs(data.gx) < gyro_threshold && fabs(data.gy) < gyro_threshold && fabs(data.gz) < gyro_threshold) {
        return Tilted; //RED LIGHT
    }


    // Check if the device is rotating (rotating movement)
    if (
        (fabs(data.gx) > gyro_threshold) + (fabs(data.gy) > gyro_threshold) + (fabs(data.gz) > gyro_threshold) >=2 &&
        acc_magnitude > g_acc/2
    ) {
        return Rotating; //BLUE LIGHT
    }

    // Otherwise, assume the device is moving (translation)
    return Moving_h; //GREEN LIGHT
}


void *_imu_thread(void *arg)
{
    (void) arg;
    // Input your code
    // 1. initial mpu6050 sensor
    // 2. Acquire sensor data every 100ms
    // 3. Determine the motion state
    // 4. notify the LED thread to display the light color through a message.
    MPU6050 mpu;
    // get mpu6050 device id
    uint8_t device_id = mpu.getDeviceID();
    // get mpu6050 device id
    printf("[IMU_THREAD] DEVICE_ID:0x%x\n", device_id);
    mpu.initialize();

    uint8_t gyro_fs = mpu.getFullScaleGyroRange();
    uint8_t accel_fs_g = mpu.getFullScaleAccelRange();
    uint16_t accel_fs_real = 1;
    float gyro_fs_convert = 1.0;

    // Convert gyroscope full scale range to conversion factor
    if (gyro_fs == MPU6050_GYRO_FS_250)
        gyro_fs_convert = 131.0;
    else if (gyro_fs == MPU6050_GYRO_FS_500)
        gyro_fs_convert = 65.5;
    else if (gyro_fs == MPU6050_GYRO_FS_1000)
        gyro_fs_convert = 32.8;
    else if (gyro_fs == MPU6050_GYRO_FS_2000)
        gyro_fs_convert = 16.4;
    else
        printf("[IMU_THREAD] Unknown GYRO_FS: 0x%x\n", gyro_fs);

    // Convert accelerometer full scale range to real value
    if (accel_fs_g == MPU6050_ACCEL_FS_2)
        accel_fs_real = g_acc * 2;
    else if (accel_fs_g == MPU6050_ACCEL_FS_4)
        accel_fs_real = g_acc * 4;
    else if (accel_fs_g == MPU6050_ACCEL_FS_8)
        accel_fs_real = g_acc * 8;
    else if (accel_fs_g == MPU6050_ACCEL_FS_16)
        accel_fs_real = g_acc * 16;
    else
        printf("[IMU_THREAD] Unknown ACCEL_FS: 0x%x\n", accel_fs_g);

    // Calculate accelerometer conversion factor
    float accel_fs_convert = 32768.0 / accel_fs_real;

    // Initialize variables
    int16_t ax, ay, az, gx, gy, gz;

    // Variables for debouncing
    static MoveState previous_state = Stationary_h;
    static int stable_count = 0;
    const int stable_threshold = 3;  // Number of readings required to confirm a stable state

    delay_ms(1000);

    while(1) {
        mpu.getMotion6(&ax, &ay, &az, &gx, &gy, &gz);
        
        MPU6050Data data;
        MoveState current_state;
        // Acquire sensor data every 100ms
        // Subtract calibration offsets from raw data
        data.ax = (ax / accel_fs_convert) - calibration_data_ax;
        data.ay = (ay / accel_fs_convert) - calibration_data_ay;
        data.az = (az / accel_fs_convert) - calibration_data_az;
        data.gx = (gx / gyro_fs_convert) - calibration_data_gx;
        data.gy = (gy / gyro_fs_convert) - calibration_data_gy;
        data.gz = (gz / gyro_fs_convert) - calibration_data_gz;

        
        // Determine the motion state
        current_state = detectMovement(data);

         // Debouncing logic
        if (current_state == previous_state) {
            stable_count++;
        } else {
            stable_count = 0;
            previous_state = current_state;
        }

        // If the state is stable for more than 5 consecutive readings (~500ms)
        if (stable_count >= stable_threshold) {
            // Send the message to the LED thread to display the correct color
            msg_t msg;

            // Send the message to the LED thread to display the correct color
            switch (current_state) {
                case Stationary_h:
                    msg.type = LED_MSG_TYPE_YELLOW;  // Horizontal stillness, yellow light
                    break;
                case Stationary_v:
                    msg.type = LED_MSG_TYPE_MAGENTA; // Vertical stillness, magenta light
                    break;
                case Tilted:
                    msg.type = LED_MSG_TYPE_RED;   // Tilted stillness, red light
                    break;
                case Rotating:
                    msg.type = LED_MSG_TYPE_BLUE;  // Rotation, blue light
                    break;
                case Moving_h:
                    msg.type = LED_MSG_TYPE_GREEN; // Moving horizontally, green light
                    break;
                case Moving_v:
                    msg.type = LED_MSG_TYPE_CYAN; // Moving vertically, cyan light
                    break;
                default:
                    msg.type = LED_MSG_TYPE_NONE;  // Default to light off
                    break;
            }

            // Send the message to the LED thread (_led_pid)
            if (msg_send(&msg, _led_pid) <= 0) {
                printf("[IMU_THREAD] Failed to send message to LED thread\n");
            }

            // Reset the stable count after sending the message
            stable_count = 0;
        }

        // Sleep for 100 ms before reading again
        delay_ms(100);
    }
    return NULL;
}

static const shell_command_t shell_commands[] = {
    { NULL, NULL, NULL }
};

int main(void)
{
    _led_pid = thread_create(stack_for_led_thread, sizeof(stack_for_led_thread), THREAD_PRIORITY_MAIN - 2,
                            THREAD_CREATE_STACKTEST, _led_thread, NULL,
                            "led_controller_thread");
    if (_led_pid <= KERNEL_PID_UNDEF) {
        printf("[MAIN] Creation of receiver thread failed\n");
        return 1;
    }
    else
    {
        printf("[MAIN] LED_PID: %d\n", _led_pid);
    }

    // create sleep thread
    // thread_create(stack_for_sleep_thread, sizeof(stack_for_sleep_thread), THREAD_PRIORITY_MAIN - 1,
    //                         THREAD_CREATE_STACKTEST, _control_thread, &_led_pid,
    //                         "sleep");
    // printf("[Main] Initialization successful - starting the shell now\n");

    thread_create(stack_for_imu_thread, sizeof(stack_for_imu_thread), THREAD_PRIORITY_MAIN - 1,
                            THREAD_CREATE_STACKTEST, _imu_thread, NULL,
                            "imu_read_thread");
    printf("[Main] Initialization successful - starting the shell now\n");
    while(1){};
    return 0;
}
