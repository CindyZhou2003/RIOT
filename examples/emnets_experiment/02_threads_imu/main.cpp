/*
 * Copyright (C) 2021 Otto-von-Guericke-Universität Magdeburg
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @ingroup     examples
 * @{
 *
 * @file
 * @brief       Blinky application
 *
 * @author      Marian Buschsieweke <marian.buschsieweke@ovgu.de>
 *
 * @}
 */

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
#include "mpu6050.h"
#define LED_GPIO GPIO12
#define THREAD_STACKSIZE        (THREAD_STACKSIZE_IDLE)
static char stack_for_imu_thread[THREAD_STACKSIZE];

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
#define g_acc (9.8)

void calibrateMPU6050(MPU6050Data &calibration_data, MPU6050 &mpu, float accel_fs_convert, float gyro_fs_convert) {
    int16_t ax, ay, az, gx, gy, gz;
    MPU6050Data sum = {0};
    int num_samples = 1000;

    for (int i = 0; i < num_samples; i++) {
        mpu.getMotion6(&ax, &ay, &az, &gx, &gy, &gz);
        sum.ax += ax;
        sum.ay += ay;
        sum.az += az;
        sum.gx += gx;
        sum.gy += gy;
        sum.gz += gz;
        delay_ms(10);
    }

    // Calculate average for calibration
    calibration_data.ax = (sum.ax / num_samples) / accel_fs_convert;
    calibration_data.ay = (sum.ay / num_samples) / accel_fs_convert;
    calibration_data.az = (sum.az / num_samples) / accel_fs_convert;
    calibration_data.gx = (sum.gx / num_samples) / gyro_fs_convert;
    calibration_data.gy = (sum.gy / num_samples) / gyro_fs_convert;
    calibration_data.gz = (sum.gz / num_samples) / gyro_fs_convert;

    printf("[CALIBRATION] Calibration data: (%.02f, %.02f, %.02f) (m/s²), (%.02f, %.02f, %.02f) (°/s)\n", 
        calibration_data.ax, calibration_data.ay, calibration_data.az, 
        calibration_data.gx, calibration_data.gy, calibration_data.gz);
}

void *_imu_thread(void *arg)
{
    (void) arg;
    // Initialize MPU6050 sensor
    MPU6050 mpu;
    // get mpu6050 device id
    uint8_t device_id = mpu.getDeviceID();
    // get mpu6050 device id
    printf("[IMU_THREAD] DEVICE_ID:0x%x\n", device_id);
    mpu.initialize();
    // Configure gyroscope and accelerometer full scale ranges
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
    // int16_t ax, ay, az, gx, gy, gz;
    delay_ms(1000);
    // Main loop
    // while (1) {
    //     // Read sensor data
    //     mpu.getMotion6(&ax, &ay, &az, &gx, &gy, &gz);
    //     delay_ms(200);    
    //     MPU6050Data data;
    //     // Convert raw sensor data to real values
    //     data.ax = ax / accel_fs_convert;
    //     data.ay = ay / accel_fs_convert;
    //     data.az = az / accel_fs_convert;
    //     data.gx = gx / gyro_fs_convert;
    //     data.gy = gy / gyro_fs_convert;
    //     data.gz = gz / gyro_fs_convert;
    //     // Print sensor data and balance angle
    //     printf("----------------------------------------\n");
    //     printf("[IMU_THREAD] (X,Y,Z):(%.02f,%.02f,%.02f)(m/s^2), (XG,YG,ZG):(%.02f,%.02f,%.02f)(°/s)\n", data.ax, data.ay, data.az, data.gx, data.gy, data.gz);
    // }
    MPU6050Data data;
    calibrateMPU6050(data,mpu,accel_fs_convert,gyro_fs_convert);
    return NULL;
}
static const shell_command_t shell_commands[] = {
    { NULL, NULL, NULL }
};


int main(void)
{
    thread_create(stack_for_imu_thread, sizeof(stack_for_imu_thread), THREAD_PRIORITY_MAIN - 1,
                            THREAD_CREATE_STACKTEST, _imu_thread, NULL,
                            "imu_read_thread");
    printf("[Main] Initialization successful - starting the shell now\n");

    char line_buf[SHELL_DEFAULT_BUFSIZE];
    shell_run(shell_commands, line_buf, SHELL_DEFAULT_BUFSIZE);
    return 0;
}
