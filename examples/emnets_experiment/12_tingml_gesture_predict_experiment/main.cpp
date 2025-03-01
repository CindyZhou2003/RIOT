#include "periph_conf.h"
#include "periph/gpio.h"
#include "periph/i2c.h"
#include "shell.h"
#include <log.h>
#include <xtimer.h>
#include "ledcontroller.hh"
#include "ztimer.h"
#include "mpu6050.h"
#include <string>
#include "msg.h"

void setup();
int predict(float *imu_data, int data_len, float threashold, int class_num);
using namespace std;
#define THREAD_STACKSIZE        (THREAD_STACKSIZE_IDLE)
static char stack_for_motion_thread[THREAD_STACKSIZE];
static char stack_for_led_thread[THREAD_STACKSIZE];
static kernel_pid_t _led_pid;
#define LED_GPIO_R GPIO26
#define LED_GPIO_G GPIO25
#define LED_GPIO_B GPIO27
#define g_acc (9.8)
#define SAMPLES_PER_GESTURE (12)

#define LED_MSG_TYPE_ISR     (0x3456)
#define LED_MSG_TYPE_NONE    (0x3110)
#define LED_MSG_TYPE_RED     (0x3111)
#define LED_MSG_TYPE_GREEN   (0x3112)
#define LED_MSG_TYPE_BLUE    (0x3113)
#define LED_MSG_TYPE_YELLOW  (0x3114)
#define LED_MSG_TYPE_WHITE   (0x3115)
#define LED_MSG_TYPE_MAGENTA (0x3116)
#define LED_MSG_TYPE_CYAN    (0x3117)

#define calibration_data_ax (0.03)
#define calibration_data_ay (-0.03)
#define calibration_data_az (9.52 - g_acc)
#define calibration_data_gx (-7.33)
#define calibration_data_gy (1.55)
#define calibration_data_gz (-3.08)

struct MPU6050Data
{
    float ax, ay, az; // acceler_x_axis, acceler_y_axis, acceler_z_axis
    float gx, gy, gz; // gyroscope_x_axis, gyroscope_y_axis, gyroscope_z_axis
};
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

        delay_ms(10);    
    }
    return NULL;
}

float gyro_fs_convert = 1.0;
float accel_fs_convert;

void get_imu_data(MPU6050 mpu, float *imu_data){
    int16_t ax, ay, az, gx, gy, gz;
    for(int i = 0; i < SAMPLES_PER_GESTURE; ++i)
    {
        /* code */
        delay_ms(20);
        mpu.getMotion6(&ax, &ay, &az, &gx, &gy, &gz);
        imu_data[i*6 + 0] = ax / accel_fs_convert - calibration_data_ax;
        imu_data[i*6 + 1] = ay / accel_fs_convert - calibration_data_ay;
        imu_data[i*6 + 2] = az / accel_fs_convert - calibration_data_az;
        imu_data[i*6 + 3] = gx / gyro_fs_convert - calibration_data_gx;
        imu_data[i*6 + 4] = gy / gyro_fs_convert - calibration_data_gy;
        imu_data[i*6 + 5] = gz / gyro_fs_convert - calibration_data_gz;
    }
} 

enum MoveState{Stationary_h, Stationary_v, Tilted, Rotating, Moving_h, Moving_v};
#define class_num (6)
void *_motion_thread(void *arg)
{
    (void) arg;
    // Initialize MPU6050 sensor
    MPU6050 mpu;
    // get mpu6050 device id
    uint8_t device_id = mpu.getDeviceID();
    printf("[IMU_THREAD] DEVICE_ID:0x%x\n", device_id);
    mpu.initialize();
    // Configure gyroscope and accelerometer full scale ranges
    uint8_t gyro_fs = mpu.getFullScaleGyroRange();
    uint8_t accel_fs_g = mpu.getFullScaleAccelRange();
    uint16_t accel_fs_real = 1;

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
    accel_fs_convert = 32768.0 / accel_fs_real;
    float imu_data[SAMPLES_PER_GESTURE * 6] = {0};
    int data_len = SAMPLES_PER_GESTURE * 6;
    delay_ms(200);
    // Main loop
    int predict_interval_ms = 200;
    int current_state = 0;
    float threshold = 0.7;
    
    // int previous_state = Stationary_h;
    // int stable_count = 0;
    // const int stable_threshold = 3;  // Number of readings required to confirm a stable state

    string motions[class_num] = {"Stationary_h","Stationary_v","Tilted", "Rotating", "Moving_h","Moving_v"};
    while (1) {
        delay_ms(predict_interval_ms);    
        // Read sensor data
        get_imu_data(mpu, imu_data);
        current_state = predict(imu_data, data_len, threshold, class_num);
        // tell the led thread to do some operations
        
        // Send the message to the LED thread to display the correct color
        msg_t msg;
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
        if (msg_send(&msg, _led_pid) <= 0) {
                printf("[IMU_THREAD] Failed to send message to LED thread\n");
            }
    
        printf("Predict: %d, %s\n", current_state, motions[current_state].c_str());

    }
    return NULL;
}



int main(int argc, char* argv[])
{
    (void)argc;
    (void)argv;
    setup();
    _led_pid = thread_create(stack_for_led_thread, sizeof(stack_for_led_thread), THREAD_PRIORITY_MAIN - 2,
                            THREAD_CREATE_STACKTEST, _led_thread, NULL,
                            "led_controller_thread");
    if (_led_pid <= KERNEL_PID_UNDEF) {
        printf("[MAIN] Creation of receiver thread failed\n");
        return 1;
    }
    thread_create(stack_for_motion_thread, sizeof(stack_for_motion_thread), THREAD_PRIORITY_MAIN - 1,
                            THREAD_CREATE_STACKTEST, _motion_thread, NULL,
                            "imu_read_thread");
    printf("[Main] Initialization successful - starting the shell now\n");
    while(1);
    return 0;
    
}
