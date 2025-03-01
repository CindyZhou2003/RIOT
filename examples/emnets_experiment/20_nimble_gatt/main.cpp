/*
 * Copyright (C) 2018 Freie Universität Berlin
 *               2018 Codecoup
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
 * @brief       BLE peripheral example using NimBLE
 *
 * Have a more detailed look at the api here:
 * https://mynewt.apache.org/latest/tutorials/ble/bleprph/bleprph.html
 *
 * More examples (not ready to run on RIOT) can be found here:
 * https://github.com/apache/mynewt-nimble/tree/master/apps
 *
 * Test this application e.g. with Nordics "nRF Connect"-App
 * iOS: https://itunes.apple.com/us/app/nrf-connect/id1054362403
 * Android: https://play.google.com/store/apps/details?id=no.nordicsemi.android.mcp
 *
 * @author      Hauke Petersen <hauke.petersen@fu-berlin.de>
 * @author      Andrzej Kaczmarek <andrzej.kaczmarek@codecoup.pl>
 * @author      Hendrik van Essen <hendrik.ve@fu-berlin.de>
 *
 * @}
 */
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include "periph/gpio.h"  
#include "periph_conf.h"
#include "board.h"
#include "clk.h"
#include "thread.h"
#include "timex.h"
#include "msg.h"
#include "ledcontroller.hh"
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
#define GATT_DEVICE_INFO_UUID                   {0x180a}
#define GATT_MANUFACTURER_NAME_UUID             {0x2a29}
#define GATT_MODEL_NUMBER_UUID                  {0x2a24}
#define LED_GPIO_R GPIO26
#define LED_GPIO_G GPIO25
#define LED_GPIO_B GPIO27
#define THREAD_STACKSIZE        (THREAD_STACKSIZE_IDLE)
#define LED_MSG_TYPE_RED     (0x3111)
#define LED_MSG_TYPE_NONE    (0x3110)
static char stack_for_led_thread[THREAD_STACKSIZE];


static kernel_pid_t _led_pid;
static int r_led_state = 0;
void *_led_thread(void *arg)
{
    (void) arg;
    LEDController led(LED_GPIO_R, LED_GPIO_G, LED_GPIO_B);
    led.change_led_color(0);
    while(1){
        printf("[LED_THREAD] WAIT\n");
        msg_t msg;
        // Wait for the message from led thread
        msg_receive(&msg);
        if (msg.type == LED_MSG_TYPE_NONE)
        {
            // TURN ON LIGHT
            led.change_led_color(0);
            r_led_state = 0;
            printf("[LED_THREAD]: LED TURN OFF!!\n");
        }
        else if (msg.type == LED_MSG_TYPE_RED)
        {
            // TURN OFF LIGHT
            led.change_led_color(1);
            r_led_state = 1;
            printf("[LED_THREAD]: LED TURN ON!!\n");
        }
    }
    return NULL;
}


/* UUID = 1bce38b3-d137-48ff-a13e-033e14c7a335 */
static const ble_uuid128_t gatt_svr_svc_rw_demo_uuid
        = {{128}, {0x15, 0xa3, 0xc7, 0x14, 0x3e, 0x03, 0x3e, 0xa1, 0xff,
                0x48, 0x37, 0xd1, 0xb3, 0x38, 0xce, 0x1b}};

/* UUID = 35f28386-3070-4f3b-ba38-27507e991762 */
static const ble_uuid128_t gatt_svr_chr_rw_demo_write_uuid
        = {{128}, {0x62, 0x17, 0x99, 0x7e, 0x50, 0x27, 0x38, 0xba, 0x3b,
                0x4f, 0x70, 0x30, 0x86, 0x83, 0xf2, 0x35}};

/* UUID = 16151413-1211-1009-0807-060504030201 */
static const ble_uuid128_t gatt_svr_chr_rw_demo_readonly_uuid
        = {{128}, {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09,
                0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16}};


static int gatt_svr_chr_access_rw_demo(
        uint16_t conn_handle, uint16_t attr_handle,
        struct ble_gatt_access_ctxt *ctxt, void *arg);
        
#define DEMO_BUFFER_SIZE 100
static char rm_demo_write_data[DEMO_BUFFER_SIZE] = "This characteristic is read- and writeable!";
#define STR_ANSWER_BUFFER_SIZE 100
static char str_answer[STR_ANSWER_BUFFER_SIZE];

/* define several bluetooth services for our device */
static const struct ble_gatt_svc_def gatt_svr_svcs[] = {
    /*
     * access_cb defines a callback for read and write access events on
     * given characteristics
     */
    {
        /* Service: Read/Write Demo */
        .type = BLE_GATT_SVC_TYPE_PRIMARY,
        .uuid = (ble_uuid_t*) &gatt_svr_svc_rw_demo_uuid.u,
        .characteristics = (struct ble_gatt_chr_def[]) { {
            /* Characteristic: Read/Write Demo write */
            .uuid = (ble_uuid_t*) &gatt_svr_chr_rw_demo_write_uuid.u,
            .access_cb = gatt_svr_chr_access_rw_demo,
            .flags = BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_WRITE,
            // .flags = BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_WRITE_NO_RSP,
        }, {
            /* Characteristic: Read/Write Demo read only */
            .uuid = (ble_uuid_t*) &gatt_svr_chr_rw_demo_readonly_uuid.u,
            .access_cb = gatt_svr_chr_access_rw_demo,
            .flags = BLE_GATT_CHR_F_READ,
        }, {
            0, /* No more characteristics in this service */
        }, }
    },
    {
        0, /* No more services */
    },
};

static int gatt_svr_chr_access_rw_demo(
        uint16_t conn_handle, uint16_t attr_handle,
        struct ble_gatt_access_ctxt *ctxt, void *arg)
{
    printf("service 'rw demo' callback triggered\n");

    (void) conn_handle;
    (void) attr_handle;
    (void) arg;

    int rc = 0;
    ble_uuid_t* write_uuid = (ble_uuid_t*) &gatt_svr_chr_rw_demo_write_uuid.u;
    ble_uuid_t* readonly_uuid = (ble_uuid_t*) &gatt_svr_chr_rw_demo_readonly_uuid.u;
    if (ble_uuid_cmp(ctxt->chr->uuid, write_uuid) == 0) {

        printf("access to characteristic 'rw demo (write)'\n");
        switch (ctxt->op) {
            case BLE_GATT_ACCESS_OP_READ_CHR:
                printf("read from characteristic\n");
                printf("current value of rm_demo_write_data: '%s'\n",
                       rm_demo_write_data);
                /* send given data to the client */
                rc = os_mbuf_append(ctxt->om, &rm_demo_write_data,
                                    strlen(rm_demo_write_data));
                break;

            case BLE_GATT_ACCESS_OP_WRITE_CHR:
                printf("write to characteristic\n");
                printf("old value of rm_demo_write_data: '%s'\n",
                       rm_demo_write_data);
                uint16_t om_len;
                om_len = OS_MBUF_PKTLEN(ctxt->om);
                /* read sent data */
                rc = ble_hs_mbuf_to_flat(ctxt->om, &rm_demo_write_data,
                                         sizeof(rm_demo_write_data), &om_len);
                /* we need to null-terminate the received string */
                rm_demo_write_data[om_len] = '\0';
                msg_t msg;
                if (rm_demo_write_data[0] > 0x00)
                {
                    msg.type = LED_MSG_TYPE_RED;
                }
                else 
                {
                    msg.type = LED_MSG_TYPE_NONE;   
                }
                if (msg_send(&msg, _led_pid) <= 0){
                    printf("[SLEEP_THREAD]: possibly lost interrupt.\n");
                }
                printf("new value of rm_demo_write_data: '%s'\n",
                       rm_demo_write_data);

                break;
            default:
                printf("unhandled operation!\n");
                rc = 1;
                break;
        }
        return rc;
    }
    else if (ble_uuid_cmp(ctxt->chr->uuid, readonly_uuid) == 0) {

        printf("access to characteristic 'rw demo (read-only)'");
        if (ctxt->op == BLE_GATT_ACCESS_OP_READ_CHR) {
            snprintf(str_answer, STR_ANSWER_BUFFER_SIZE,
                     "LED STATE(R): %d", r_led_state);
            printf("%s\n", str_answer);
            rc = os_mbuf_append(ctxt->om, &str_answer, strlen(str_answer));
            return rc;
        }
        return 0;
    }

    printf("unhandled uuid!\n");
    return 1;
}

int main(void)
{
    printf("NimBLE GATT Server Example \n");

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

    int rc = 0;
    (void)rc;

    /* verify and add our custom services */
    rc = ble_gatts_count_cfg(gatt_svr_svcs);
    assert(rc == 0);
    rc = ble_gatts_add_svcs(gatt_svr_svcs);
    assert(rc == 0);

    /* set the device name */
    ble_svc_gap_device_name_set(CONFIG_NIMBLE_AUTOADV_DEVICE_NAME);
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

    return 0;
}
