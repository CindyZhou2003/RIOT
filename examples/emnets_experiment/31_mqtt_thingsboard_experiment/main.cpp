/*
 * Copyright (C) 2019 Javier FILEIV <javier.fileiv@gmail.com>
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @ingroup     examples
 * @{
 *
 * @file        main.c
 * @brief       Example using MQTT Paho package from RIOT
 *
 * @author      Javier FILEIV <javier.fileiv@gmail.com>
 *
 * @}
 */

#include <cassert>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include "timex.h"
#include "ztimer.h"
#include "shell.h"
#include "thread.h"
#include "mutex.h"
#include "paho_mqtt.h"
#include "MQTTClient.h"
#include "xtimer.h"
#include <string>
#include "ledcontroller.hh"
#include "mpu6050.h"
#include "periph/gpio.h"  
#include "periph_conf.h"
#include "board.h"
#include "clk.h"
#include "msg.h"
#include "lwip/netif.h"

extern "C" {
    #include "nimble_riot.h"
    #include "nimble_autoadv.h"
    #include "log.h"
    #include "host/ble_hs.h"
    #include "host/util/util.h"
    #include "host/ble_gatt.h"
    #include "services/gap/ble_svc_gap.h"
    #include "services/gatt/ble_svc_gatt.h"
}
using namespace std;
void setup();
int predict(float *imu_data, int data_len, float threashold, int class_num);

#define MAIN_QUEUE_SIZE     (8)
static msg_t _main_msg_queue[MAIN_QUEUE_SIZE];

#define BUF_SIZE                        1024
#define MQTT_VERSION_v311               4       /* MQTT v3.1.1 version is 4 */
#define COMMAND_TIMEOUT_MS              4000

string DEFAULT_MQTT_CLIENT_ID = "esp32_test";
string DEFAULT_MQTT_USER = "esp32";
string DEFAULT_MQTT_PWD = "esp32";
// Please enter the IP of the computer on which you have ThingsBoard installed.
string DEFAULT_IPV4 = "192.168.43.173";
string DEFAULT_TOPIC = "v1/devices/me/telemetry";


/**
 * @brief Default MQTT port
 */
#define DEFAULT_MQTT_PORT               1883

/**
 * @brief Keepalive timeout in seconds
 */
#define DEFAULT_KEEPALIVE_SEC           10

#ifndef MAX_LEN_TOPIC
#define MAX_LEN_TOPIC                   100
#endif

#ifndef MAX_TOPICS
#define MAX_TOPICS                      4
#endif

#define IS_CLEAN_SESSION                1
#define IS_RETAINED_MSG                 0

static MQTTClient client;
static Network network;
static int topic_cnt = 0;

// 获取当前时间戳（单位：毫秒）
#define TIME_NOW_MS() (ztimer_now(ZTIMER_USEC) / 1000)
#define LED_GPIO_R GPIO26
#define LED_GPIO_G GPIO25
#define LED_GPIO_B GPIO27
#define g_acc (9.8)
#define SAMPLES_PER_GESTURE (12)
#define THREAD_STACKSIZE        (THREAD_STACKSIZE_IDLE)
#define STR_ANSWER_BUFFER_SIZE 50
static char stack_for_led_thread[THREAD_STACKSIZE];
static char stack_for_motion_thread[THREAD_STACKSIZE*2];
// the pid of led thread
static kernel_pid_t _led_pid;
// static kernel_pid_t _main_pid;
static kernel_pid_t _motion_pid;

#define calibration_data_ax (0.03)
#define calibration_data_ay (-0.03)
#define calibration_data_az (9.52 - g_acc)
#define calibration_data_gx (-7.33)
#define calibration_data_gy (1.55)
#define calibration_data_gz (-3.08)

#define LED_MSG_TYPE_ISR     (0x3456)
#define LED_MSG_TYPE_NONE    (0x3110)
#define LED_MSG_TYPE_RED     (0x3111) //12561
#define LED_MSG_TYPE_GREEN   (0x3112) //12562
#define LED_MSG_TYPE_BLUE    (0x3113) //12563
#define LED_MSG_TYPE_YELLOW  (0x3114) //12564
#define LED_MSG_TYPE_WHITE   (0x3115) //12565
#define LED_MSG_TYPE_MAGENTA (0x3116) //12566
#define LED_MSG_TYPE_CYAN    (0x3117) //12567
// 声明全局变量

MPU6050 mpu;
static char str_answer[STR_ANSWER_BUFFER_SIZE];  // 供 BLE 回调函数使用的字符数组
static std::string motions[] = {"Stationary_h", "Stationary_v", "Tilted", "Rotating", "Moving_h", "Moving_v"};
static int current_state = 0;  // 当前预测的运动状态
static float threshold = 0.7;  // 运动预测模型的阈值
static int collect_interval_ms = 100;  // 数据采集频率
static int current_led_state = LED_MSG_TYPE_NONE; // 当前 LED 的状态
static int mqtt_interval_ms = 5000; //上传云端间隔时间
// Declare LEDController globally
LEDController led(LED_GPIO_R, LED_GPIO_G, LED_GPIO_B);

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

void *_led_thread(void *arg){
    (void) arg;
    msg_t msg;
    // Wait for the message from main thread
    msg_receive(&msg);
    printf("[LED_THREAD] main Sender_PID: %d\n", msg.sender_pid);

    while(1){
        // Input your codes
        // Wait for a message to control the LED
        // Display different light colors based on the motion state of the device.
        printf("[LED_THREAD] WAIT\n");
        msg_t msg;
        // Wait for the message from the sleep thread
        msg_receive(&msg);
        printf("[LED_THREAD] Sender_PID: %d\n", msg.sender_pid);

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

enum MoveState{Stationary_h, Stationary_v, Tilted, Rotating, Moving_h, Moving_v};
#define class_num (6)

void get_imu_data(MPU6050 mpu, float *imu_data){
    int16_t ax, ay, az, gx, gy, gz;
    for(int i = 0; i < SAMPLES_PER_GESTURE; ++i)
    {
        /* code */
        delay_ms(collect_interval_ms);
        mpu.getMotion6(&ax, &ay, &az, &gx, &gy, &gz);
        imu_data[i*6 + 0] = ax / accel_fs_convert - calibration_data_ax;
        imu_data[i*6 + 1] = ay / accel_fs_convert - calibration_data_ay;
        imu_data[i*6 + 2] = az / accel_fs_convert - calibration_data_az;
        imu_data[i*6 + 3] = gx / gyro_fs_convert - calibration_data_gx;
        imu_data[i*6 + 4] = gy / gyro_fs_convert - calibration_data_gy;
        imu_data[i*6 + 5] = gz / gyro_fs_convert - calibration_data_gz;
    }
}

void *_motion_thread(void *arg){
    (void) arg;
    // MPU6050 mpu;
    
    // get mpu6050 device id
    uint8_t device_id = mpu.getDeviceID();
    printf("[IMU_THREAD] DEVICE_ID:0x%x\n", device_id);
    // Initialize MPU6050 sensor
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
    
    string motions[class_num] = {"Stationary_h","Stationary_v","Tilted", "Rotating", "Moving_h","Moving_v"};

    while (1) {
        delay_ms(collect_interval_ms); 

        // Read sensor data
        get_imu_data(mpu, imu_data);
        current_state = predict(imu_data, data_len, threshold, class_num);
        // 记录预测间隔
        printf("[MOTION_THREAD] Interval ms: %d ms\n",collect_interval_ms);

        // Send the message to the LED thread to display the correct color
        msg_t msg;
        switch (current_state) {
            case Stationary_h:
                msg.type = LED_MSG_TYPE_YELLOW;  // Horizontal stillness, yellow light
                current_led_state = LED_MSG_TYPE_YELLOW;
                break;
            case Stationary_v:
                msg.type = LED_MSG_TYPE_MAGENTA; // Vertical stillness, magenta light
                current_led_state = LED_MSG_TYPE_MAGENTA;
                break;
            case Tilted:
                msg.type = LED_MSG_TYPE_RED;   // Tilted stillness, red light
                current_led_state = LED_MSG_TYPE_RED;
                break;
            case Rotating:
                msg.type = LED_MSG_TYPE_BLUE;  // Rotation, blue light
                current_led_state = LED_MSG_TYPE_BLUE;
                break;
            case Moving_h:
                msg.type = LED_MSG_TYPE_GREEN; // Moving horizontally, green light
                current_led_state = LED_MSG_TYPE_GREEN;
                break;
            case Moving_v:
                msg.type = LED_MSG_TYPE_CYAN; // Moving vertically, cyan light
                current_led_state = LED_MSG_TYPE_CYAN;
                break;
            default:
                msg.type = LED_MSG_TYPE_NONE;  // Default to light off
                current_led_state = LED_MSG_TYPE_NONE;
                break;
        }
        if (msg_send(&msg, _led_pid) <= 0) {
                printf("[IMU_THREAD] Failed to send message to LED thread\n");
            }
    
        printf("Predict: %d, %s\n", current_state, motions[current_state].c_str());

    }
    return NULL;
}

/* define several bluetooth services for our device */
static int gatt_svr_chr_access_rw_demo(
        uint16_t conn_handle, uint16_t attr_handle,
        struct ble_gatt_access_ctxt *ctxt, void *arg);
/* UUIDs */
/* UUID = 3e38a637-d13b-4a99-b13e-0307b3a27357 */
static const ble_uuid128_t gatt_svr_svc_motion_state_uuid 
        = {{128}, {0x57, 0x73, 0xa2, 0xb3, 0x07, 0x03, 0x3e, 0xb1, 0x99,
                0x4a, 0x3b, 0xd1, 0x37, 0xa6, 0x38, 0x3e}};

/* UUID = f2e1d5c3-4c9b-43de-8d5c-3e021b24394d */
static const ble_uuid128_t gatt_svr_chr_motion_state_uuid 
        = {{128}, {0x4d, 0x39, 0x24, 0x1b, 0x02, 0x3e, 0x5c, 0x8d, 0xde,
                0x43, 0x9b, 0x4c, 0xc3, 0xd5, 0xe1, 0xf2}};// current state

/* UUID = 3a6f1e78-ae4c-4a45-853c-0e50d3d4070e */
static const ble_uuid128_t gatt_svr_chr_threshold_uuid
        = {{128}, {0x0e, 0x07, 0xd4, 0x3d, 0x50, 0x0e, 0x3c, 0x85, 0x45,
                0x4a, 0x4c, 0xae, 0x78, 0x1e, 0x6f, 0x3a}};// forecasting threshold

/* UUID = 9b6c7a6b-938c-4f17-a2f9-33579e97c9b7 */
static const ble_uuid128_t gatt_svr_chr_sampling_rate_uuid
        = {{128}, {0xb7, 0xc9, 0x97, 0x9e, 0x57, 0x33, 0xf9, 0xa2, 0x17,
                0x4f, 0x8c, 0x93, 0x6b, 0x7a, 0x6c, 0x9b}};// collecting data interval

/* UUID = 2bdae6d9-7f72-4779-98c8-4b8d4e523745 */
static const ble_uuid128_t gatt_svr_chr_led_control_uuid
        = {{128}, {0x45, 0x37, 0x52, 0x4e, 0x8d, 0x4b, 0xc8, 0x98, 0x79,
                0x47, 0x72, 0x7f, 0xd9, 0xe6, 0xda, 0x2b}};// change led light

/* UUID = 6f32b4b7-55f8-4532-bc21-4cb99fa2c9f7 */
static const ble_uuid128_t gatt_svr_chr_mqtt_interval_uuid 
        = {{128}, {0xf7, 0xc9, 0xa2, 0x9f, 0xb9, 0x4c, 0x21, 0xbc, 0x32,
                0x45, 0xf8, 0x55, 0xb7, 0xb4, 0x32, 0x6f}};  // MQTT Interval characteristic UUID

// Define the BLE GATT server structure to include the new characteristic
static const struct ble_gatt_svc_def gatt_svr_svcs[] = {
    {
        .type = BLE_GATT_SVC_TYPE_PRIMARY,
        .uuid = (ble_uuid_t*) &gatt_svr_svc_motion_state_uuid.u,
        .characteristics = (struct ble_gatt_chr_def[]) { {
            .uuid = (ble_uuid_t*) &gatt_svr_chr_motion_state_uuid.u,
            .access_cb = gatt_svr_chr_access_rw_demo,
            .flags = BLE_GATT_CHR_F_READ,
        }, {
            .uuid = (ble_uuid_t*) &gatt_svr_chr_threshold_uuid.u,
            .access_cb = gatt_svr_chr_access_rw_demo,
            .flags = BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_WRITE,
        }, {
            .uuid = (ble_uuid_t*) &gatt_svr_chr_sampling_rate_uuid.u,
            .access_cb = gatt_svr_chr_access_rw_demo,
            .flags = BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_WRITE,
        }, {
            .uuid = (ble_uuid_t*) &gatt_svr_chr_led_control_uuid.u,
            .access_cb = gatt_svr_chr_access_rw_demo,
            .flags = BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_WRITE,
        }, {
            .uuid = (ble_uuid_t*) &gatt_svr_chr_mqtt_interval_uuid.u,   // New characteristic
            .access_cb = gatt_svr_chr_access_rw_demo,
            .flags = BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_WRITE,
        }, {
            0, /* No more characteristics */
        }},
    },
    {
        0, /* No more services */
    },
};

// 增加全局变量，用于保存 BLE 读取的数据格式
static char str_threshold[STR_ANSWER_BUFFER_SIZE];  // 存储阈值的字符串表示
static char str_collect_interval[STR_ANSWER_BUFFER_SIZE];  // 存储采样频率的字符串表示
static char str_led_state[STR_ANSWER_BUFFER_SIZE]; //存储led灯颜色
static char str_upload_interval[STR_ANSWER_BUFFER_SIZE];

static int gatt_svr_chr_access_rw_demo(
        uint16_t conn_handle, uint16_t attr_handle,
        struct ble_gatt_access_ctxt *ctxt, void *arg) {

    int rc = 0;
    printf("[GATT_SVR] Time: %lu ms, Operation: %s, Attribute Handle: 0x%04X\n",
           TIME_NOW_MS(), (ctxt->op == BLE_GATT_ACCESS_OP_READ_CHR) ? "READ" : "WRITE", attr_handle);

    if (ble_uuid_cmp(ctxt->chr->uuid, &gatt_svr_chr_motion_state_uuid.u) == 0) {
        if (ctxt->op == BLE_GATT_ACCESS_OP_READ_CHR) {
            snprintf(str_answer, STR_ANSWER_BUFFER_SIZE, "Current Motion State: %s", motions[current_state].c_str());
            rc = os_mbuf_append(ctxt->om, str_answer, strlen(str_answer));
        }
    }
    else if (ble_uuid_cmp(ctxt->chr->uuid, &gatt_svr_chr_threshold_uuid.u) == 0) {
        if (ctxt->op == BLE_GATT_ACCESS_OP_WRITE_CHR) {
            uint16_t om_len = OS_MBUF_PKTLEN(ctxt->om);
            char buffer[20] = {0};  // 创建一个临时字符缓冲区
            rc = ble_hs_mbuf_to_flat(ctxt->om, buffer, sizeof(buffer), &om_len);
            if (rc == 0) {
                // 将字符串转换为 float
                threshold = atof(buffer);
                printf("[GATT_SVR]: Updated threshold value: %.2f\n", threshold);
            }
        }  else if (ctxt->op == BLE_GATT_ACCESS_OP_READ_CHR) {
            // 格式化为字符串表示
            snprintf(str_threshold, sizeof(str_threshold), "%.2f", threshold);
            rc = os_mbuf_append(ctxt->om, str_threshold, strlen(str_threshold));
        }
    }
    else if (ble_uuid_cmp(ctxt->chr->uuid, &gatt_svr_chr_sampling_rate_uuid.u) == 0) {
        if (ctxt->op == BLE_GATT_ACCESS_OP_WRITE_CHR) {
            uint16_t om_len = OS_MBUF_PKTLEN(ctxt->om);
            rc = ble_hs_mbuf_to_flat(ctxt->om, &collect_interval_ms, sizeof(collect_interval_ms), &om_len);
        } else if (ctxt->op == BLE_GATT_ACCESS_OP_READ_CHR) {
            // 格式化为字符串表示
            snprintf(str_collect_interval, sizeof(str_collect_interval), "%d ms", collect_interval_ms);
            rc = os_mbuf_append(ctxt->om, str_collect_interval, strlen(str_collect_interval));
        }
    }// 处理LED控制特性
    else if (ble_uuid_cmp(ctxt->chr->uuid, &gatt_svr_chr_led_control_uuid.u) == 0) {
        if (ctxt->op == BLE_GATT_ACCESS_OP_WRITE_CHR) {
            uint16_t om_len = OS_MBUF_PKTLEN(ctxt->om);
            int led_color = 0;
            rc = ble_hs_mbuf_to_flat(ctxt->om, &led_color, sizeof(led_color), &om_len);
            
            // 根据写入的 LED 颜色代码控制 LED 灯颜色
            msg_t msg;
            msg.type = led_color;

            // 检查消息发送结果并打印调试信息
            if (msg_send(&msg, _led_pid) > 0) {
                printf("[GATT_SVR]: Successfully sent LED control message: %d\n", led_color);
                // 更新全局 LED 状态变量
                current_led_state = led_color;
            } else {
                printf("[GATT_SVR]: Failed to send LED control message.\n");
                rc = 1; // 错误代码
            }
        } 
        else if (ctxt->op == BLE_GATT_ACCESS_OP_READ_CHR) {
            // 读取当前 LED 的状态，格式化为字符串表示
            switch (current_led_state) {
                case LED_MSG_TYPE_NONE:
                    snprintf(str_led_state, STR_ANSWER_BUFFER_SIZE, "LED OFF");
                    break;
                case LED_MSG_TYPE_RED:
                    snprintf(str_led_state, STR_ANSWER_BUFFER_SIZE, "RED");
                    break;
                case LED_MSG_TYPE_GREEN:
                    snprintf(str_led_state, STR_ANSWER_BUFFER_SIZE, "GREEN");
                    break;
                case LED_MSG_TYPE_BLUE:
                    snprintf(str_led_state, STR_ANSWER_BUFFER_SIZE, "BLUE");
                    break;
                case LED_MSG_TYPE_YELLOW:
                    snprintf(str_led_state, STR_ANSWER_BUFFER_SIZE, "YELLOW");
                    break;
                case LED_MSG_TYPE_WHITE:
                    snprintf(str_led_state, STR_ANSWER_BUFFER_SIZE, "WHITE");
                    break;
                case LED_MSG_TYPE_MAGENTA:
                    snprintf(str_led_state, STR_ANSWER_BUFFER_SIZE, "MAGENTA");
                    break;
                case LED_MSG_TYPE_CYAN:
                    snprintf(str_led_state, STR_ANSWER_BUFFER_SIZE, "CYAN");
                    break;
                default:
                    snprintf(str_led_state, STR_ANSWER_BUFFER_SIZE, "UNKNOWN");
                    break;
            }

            // 将字符串形式的 LED 状态返回给客户端
            rc = os_mbuf_append(ctxt->om, str_led_state, strlen(str_led_state));
            if (rc != 0) {
                printf("[GATT_SVR]: Failed to send LED state to client.\n");
            }
        }
    
    }//上传云端间隔时间
    else if (ble_uuid_cmp(ctxt->chr->uuid, &gatt_svr_chr_mqtt_interval_uuid.u) == 0) {
        if (ctxt->op == BLE_GATT_ACCESS_OP_WRITE_CHR) {
        uint16_t om_len = OS_MBUF_PKTLEN(ctxt->om);
        rc = ble_hs_mbuf_to_flat(ctxt->om, &mqtt_interval_ms, sizeof(mqtt_interval_ms), &om_len);
        printf("[GATT_SVR] Updated mqtt_interval_ms: %d ms\n", mqtt_interval_ms);
        } 
        else if (ctxt->op == BLE_GATT_ACCESS_OP_READ_CHR) {
        snprintf(str_upload_interval, sizeof(str_upload_interval), "%d ms", mqtt_interval_ms);
        rc = os_mbuf_append(ctxt->om, str_upload_interval, strlen(str_upload_interval));
        }
    }

    return rc;
}

int mqtt_disconnect(void){
    topic_cnt = 0;
    int res = MQTTDisconnect(&client);
    if (res < 0) {
        printf("mqtt_example: Unable to disconnect\n");
    }
    else {
        printf("mqtt_example: Disconnect successful\n");
    }

    NetworkDisconnect(&network);
    return res;
}

int mqtt_connect(void)
{
    const char *remote_ip;
    remote_ip = DEFAULT_IPV4.c_str();
    if (client.isconnected) {
        printf("mqtt_example: client already connected, disconnecting it\n");
        MQTTDisconnect(&client);
        NetworkDisconnect(&network);
    }
    int port = DEFAULT_MQTT_PORT;

    MQTTPacket_connectData data = MQTTPacket_connectData_initializer;
    data.MQTTVersion = MQTT_VERSION_v311;

    data.clientID.cstring = (char *)DEFAULT_MQTT_CLIENT_ID.c_str();
    data.username.cstring = (char *)DEFAULT_MQTT_USER.c_str();
    data.password.cstring = (char *)DEFAULT_MQTT_PWD.c_str();
    data.keepAliveInterval = DEFAULT_KEEPALIVE_SEC;
    data.cleansession = IS_CLEAN_SESSION;
    data.willFlag = 0;

    NetworkConnect(&network, (char *)remote_ip, port);
    int ret = MQTTConnect(&client, &data);
    if (ret < 0) {
        printf("mqtt_example: Unable to connect client %d\n", ret);
        mqtt_disconnect();
        return ret;
    }
    else {
        printf("mqtt_example: Connection successfully\n");
    }

    return (ret > 0) ? 0 : 1;
}

int mqtt_pub(void)
{
    enum QoS qos = QOS0;

    // Prepare the message structure
    MQTTMessage message;
    message.qos = qos;
    message.retained = IS_RETAINED_MSG;

    // Gather MPU6050 data
    MPU6050Data sensor_data;
    int16_t ax, ay, az, gx, gy, gz;
    // MPU6050 mpu;
    // mpu.initialize();
    mpu.getMotion6(&ax, &ay, &az, &gx, &gy, &gz);
    sensor_data.ax = ax / accel_fs_convert - calibration_data_ax;
    sensor_data.ay = ay / accel_fs_convert - calibration_data_ay;
    sensor_data.az = az / accel_fs_convert - calibration_data_az;
    sensor_data.gx = gx / gyro_fs_convert - calibration_data_gx;
    sensor_data.gy = gy / gyro_fs_convert - calibration_data_gy;
    sensor_data.gz = gz / gyro_fs_convert - calibration_data_gz;

    // Prepare JSON payload with proper variable names for ThingsBoard
    char json[512];  // Adjust size based on expected data size
    snprintf(json, sizeof(json),
             "{\"ax\": %.2f, \"ay\": %.2f, \"az\": %.2f, "
             "\"gx\": %.2f, \"gy\": %.2f, \"gz\": %.2f, "
             "\"motion_state\": \"%d\", \"r_led\": %d, \"g_led\": %d, \"b_led\": %d}",
             sensor_data.ax, sensor_data.ay, sensor_data.az,
             sensor_data.gx, sensor_data.gy, sensor_data.gz,
             current_state, 
             led.get_r_value(),   // Get Red LED value
             led.get_g_value(),   // Get Green LED value
             led.get_b_value());  // Get Blue LED value

    printf("[MQTT PUB] Payload: %s\n", json);

    // Set the message payload and payload length
    message.payload = json;
    message.payloadlen = strlen((char *)message.payload);

    // Publish the message to the MQTT broker
    int rc;
    if ((rc = MQTTPublish(&client, DEFAULT_TOPIC.c_str(), &message)) < 0) {
        printf("mqtt_example: Unable to publish (%d)\n", rc);
    }
    else {
        printf("mqtt_example: Message (%s) has been published to topic %s with QOS %d\n",
               (char *)message.payload, DEFAULT_TOPIC.c_str(), (int)message.qos);
    }

    return rc;
}

void send(void)
{
    mqtt_connect();
    mqtt_pub();
    mqtt_disconnect();
}


static unsigned char buf[BUF_SIZE];
static unsigned char readbuf[BUF_SIZE];

int main(void)
{
    if (IS_USED(MODULE_GNRC_ICMPV6_ECHO)) {
        msg_init_queue(_main_msg_queue, MAIN_QUEUE_SIZE);
    }

#ifdef MODULE_LWIP
    printf("Initializing LWIP...\n");
    delay_ms(100);
    // Ensure Wi-Fi initialization code here is successful
#endif

    NetworkInit(&network);
    MQTTClientInit(&client, &network, COMMAND_TIMEOUT_MS, buf, BUF_SIZE,
                   readbuf,
                   BUF_SIZE);
    printf("Running mqtt paho example. Type help for commands info\n");

    MQTTStartTask(&client); 

    // create led thread
    _led_pid = thread_create(stack_for_led_thread, sizeof(stack_for_led_thread), THREAD_PRIORITY_MAIN - 1,
                            THREAD_CREATE_STACKTEST, _led_thread, NULL,
                            "led");
    if (_led_pid <= KERNEL_PID_UNDEF) {
        printf("[MAIN] Creation of receiver thread failed\n");
        return 1;
    }
    else{
        printf("[MAIN] LED_PID: %d\n", _led_pid);
    }

    setup();

    delay_ms(500);  // Adjust delay as needed
    // waiting for get IP 
    extern struct netif *netif_default;
    uint32_t addr;
    do
    {
        addr = netif_ip_addr4(netif_default)->addr;
        printf("Waiting for getting IP, current IP addr:");
        ip_addr_debug_print(LWIP_DBG_ON, netif_ip_addr4(netif_default));
        printf("\n");
        delay_ms(1000);
    }while (addr == 0x0);

    // create motion thread
    _motion_pid = thread_create(stack_for_motion_thread, sizeof(stack_for_motion_thread), THREAD_PRIORITY_MAIN - 2,
                            THREAD_CREATE_STACKTEST, _motion_thread, NULL,
                            "motion_predict_thread");
    if (_motion_pid <= KERNEL_PID_UNDEF) {
        printf("[MAIN] Creation of receiver thread failed\n");
        return 1;
    }
    else{
        printf("[MAIN] MOTION_PID: %d\n", _motion_pid);
    }

    int rc = 0;
    (void)rc;

    /* verify and add our custom services */
    rc = ble_gatts_count_cfg(gatt_svr_svcs);
    assert(rc == 0);
    rc = ble_gatts_add_svcs(gatt_svr_svcs);
    assert(rc == 0);

    /* set the device name */
    ble_svc_gap_device_name_set("ESP32");
    /* reload the GATT server to link our added services */
    ble_gatts_start();

    // 获取蓝牙设备的默认 MAC 地址
    uint8_t own_addr_type;
    uint8_t own_addr[6];
    ble_hs_id_infer_auto(0, &own_addr_type);
    ble_hs_id_copy_addr(own_addr_type, own_addr, NULL);

    // 打印 MAC 地址
    LOG_INFO("Default MAC address: %02X:%02X:%02X:%02X:%02X:%02X\n",
             own_addr[5], own_addr[4], own_addr[3],
             own_addr[2], own_addr[1], own_addr[0]);
    /* start to advertise this node */
    nimble_autoadv_start(NULL);  

    while (1)
    {
        send();
        printf("[THINGSBOARD] Uploaded mqtt_interval_ms: %d ms\n", mqtt_interval_ms);
        delay_ms(mqtt_interval_ms);     
    }
    
    return 0;
}
