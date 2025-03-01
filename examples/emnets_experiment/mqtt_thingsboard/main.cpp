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
#include "lwip/netif.h"

using namespace std;

#define MAIN_QUEUE_SIZE     (8)
static msg_t _main_msg_queue[MAIN_QUEUE_SIZE];

#define BUF_SIZE                        1024
#define MQTT_VERSION_v311               4       /* MQTT v3.1.1 version is 4 */
#define COMMAND_TIMEOUT_MS              4000

string DEFAULT_MQTT_CLIENT_ID = "esp32_test";
string DEFAULT_MQTT_USER = "esp32";
string DEFAULT_MQTT_PWD = "esp32";
// Please enter the IP of the computer on which you have ThingsBoard installed.
string DEFAULT_IPV4 = "192.168.69.82";
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
static char _topic_to_subscribe[MAX_TOPICS][MAX_LEN_TOPIC];
static int led_state;
#define THREAD_STACKSIZE        (THREAD_STACKSIZE_IDLE)

static char stack_for_led_thread[THREAD_STACKSIZE];
#define LED_MSG_TYPE_ISR     (0x3456)
#define LED_MSG_TYPE_RED     (0x3111)
#define LED_MSG_TYPE_NONE    (0x3110)
#define LED_GPIO_R GPIO26
#define LED_GPIO_G GPIO25
#define LED_GPIO_B GPIO27
// the pid of led thread
static kernel_pid_t _led_pid;
static kernel_pid_t _main_pid;

void delay_ms(uint32_t sleep_ms)
{
    ztimer_sleep(ZTIMER_USEC, sleep_ms * US_PER_MS);
    return;
}

void *_led_thread(void *arg)
{
    (void) arg;
    // init led controller
    LEDController led(LED_GPIO_R, LED_GPIO_G, LED_GPIO_B);
    msg_t msg;
    // Wait for the message from main thread
    msg_receive(&msg);
    printf("[LED_THREAD] main Sender_PID: %d\n", msg.sender_pid);
    while(1){
        printf("[LED_THREAD] WAIT\n");
        msg_t msg;
        // Wait for the message from other thread
        msg_receive(&msg);
        _main_pid = msg.sender_pid;
        printf("[LED_THREAD] Sender_PID: %d\n", msg.sender_pid);

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

static unsigned get_qos(const char *str)
{
    int qos = atoi(str);

    switch (qos) {
    case 1:     return QOS1;
    case 2:     return QOS2;
    default:    return QOS0;
    }
}

static void _on_msg_received(MessageData *data)
{
    printf("paho_mqtt_example: message received on topic"
           " %.*s: %.*s\n",
           (int)data->topicName->lenstring.len,
           data->topicName->lenstring.data, (int)data->message->payloadlen,
           (char *)data->message->payload);
}

int mqtt_disconnect(void)
{
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

    MQTTMessage message;
    message.qos = qos;
    message.retained = IS_RETAINED_MSG;
    led_state = !led_state;
    char json[100];  
    snprintf(json, 100, "{r_led_state:%d}", led_state);
    printf("[Send] Message:%s\n", json);
    message.payload = json;
    message.payloadlen = strlen((char *)message.payload);

    int rc;
    if ((rc = MQTTPublish(&client, DEFAULT_TOPIC.c_str(), &message)) < 0) {
        printf("mqtt_example: Unable to publish (%d)\n", rc);
    }
    else {
        printf("mqtt_example: Message (%s) has been published to topic %s"
               "with QOS %d\n",
               (char *)message.payload, DEFAULT_TOPIC.c_str(), (int)message.qos);
    }

    return rc;
}

static int _cmd_sub(int argc, char **argv)
{
    enum QoS qos = QOS0;

    if (argc < 2) {
        printf("usage: %s <topic name> [QoS level]\n", argv[0]);
        return 1;
    }

    if (argc >= 3) {
        qos = static_cast<QoS>(get_qos(argv[2]));
    }

    if (topic_cnt > MAX_TOPICS) {
        printf("mqtt_example: Already subscribed to max %d topics,"
                "call 'unsub' command\n", topic_cnt);
        return -1;
    }

    if (strlen(argv[1]) > MAX_LEN_TOPIC) {
        printf("mqtt_example: Not subscribing, topic too long %s\n", argv[1]);
        return -1;
    }
    strncpy(_topic_to_subscribe[topic_cnt], argv[1], strlen(argv[1]));

    printf("mqtt_example: Subscribing to %s\n", _topic_to_subscribe[topic_cnt]);
    int ret = MQTTSubscribe(&client,
              _topic_to_subscribe[topic_cnt], qos, _on_msg_received);
    if (ret < 0) {
        printf("mqtt_example: Unable to subscribe to %s (%d)\n",
               _topic_to_subscribe[topic_cnt], ret);
        mqtt_disconnect();
    }
    else {
        printf("mqtt_example: Now subscribed to %s, QOS %d\n",
               argv[1], (int) qos);
        topic_cnt++;
    }
    return ret;
}

static int _cmd_unsub(int argc, char **argv)
{
    if (argc < 2) {
        printf("usage %s <topic name>\n", argv[0]);
        return 1;
    }

    int ret = MQTTUnsubscribe(&client, argv[1]);

    if (ret < 0) {
        printf("mqtt_example: Unable to unsubscribe from topic: %s\n", argv[1]);
        mqtt_disconnect();
    }
    else {
        printf("mqtt_example: Unsubscribed from topic:%s\n", argv[1]);
        topic_cnt--;
    }
    return ret;
}
static int _cmd_send(int argc, char **argv)
{
    (void) argc;
    (void) argv;

    for (int i = 0; i < 10; ++i)
    {
        msg_t msg;
        msg.type = led_state == 0 ? LED_MSG_TYPE_NONE : LED_MSG_TYPE_RED;
        if (msg_send(&msg, _led_pid) <= 0){
            printf("[_cmd_send]: possibly lost interrupt.\n");
        }
        else{
            printf("[_cmd_send]: Successfully set interrupt.\n");
        }
        mqtt_connect();
        mqtt_pub();
        mqtt_disconnect();
        delay_ms(2000);
    }
    return 0;
}


void send(void)
{
    mqtt_connect();
    mqtt_pub();
    mqtt_disconnect();
}
static const shell_command_t shell_commands[] =
{
    // { "ip",     "set_ip",                             _cmd_ip     },
    { "sub",    "subscribe topic",                    _cmd_sub    },
    { "unsub",  "unsubscribe from topic",             _cmd_unsub  },
    {  "send",  "send data",                          _cmd_send },
    { NULL,     NULL,                                 NULL        }
};

static unsigned char buf[BUF_SIZE];
static unsigned char readbuf[BUF_SIZE];

int main(void)
{
    if (IS_USED(MODULE_GNRC_ICMPV6_ECHO)) {
        msg_init_queue(_main_msg_queue, MAIN_QUEUE_SIZE);
    }
#ifdef MODULE_LWIP
    /* let LWIP initialize */
    delay_ms(10);
#endif

    NetworkInit(&network);
    MQTTClientInit(&client, &network, COMMAND_TIMEOUT_MS, buf, BUF_SIZE,
                   readbuf,
                   BUF_SIZE);
    printf("Running mqtt paho example. Type help for commands info\n");

    MQTTStartTask(&client);
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
    msg_t msg_test;
    msg_test.type = LED_MSG_TYPE_RED;
    if (msg_send(&msg_test, _led_pid) <= 0){
        printf("[main]: possibly lost interrupt.\n");
    }
    else{
        printf("[main]: Successfully set interrupt.\n");
    }

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


    char line_buf[SHELL_DEFAULT_BUFSIZE];
    shell_run(shell_commands, line_buf, SHELL_DEFAULT_BUFSIZE);

    return 0;
}
