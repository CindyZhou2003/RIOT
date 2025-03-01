APPLICATION = final_project

# If no BOARD is found in the environment, use this default:
# BOARD ?= esp32s3-devkit
BOARD ?= esp32-wroom-32
# TensorFlow-Lite crashes on M4/M7 CPUs when FPU is enabled, so disable it by
# default for now
DISABLE_MODULE += cortexm_fpu
USEPKG += tflite-micro
USEPKG += nimble
# Include NimBLE
FEATURES_REQUIRED += periph_i2c
FEATURES_OPTIONAL += periph_i2c_reconfigure
EXTERNAL_MODULE_DIRS += external_modules
USEMODULE += gesture
USEMODULE += nimble_svc_gap
USEMODULE += nimble_svc_gatt
USEMODULE += nimble_autoadv

# This has to be the absolute path to the RIOT base directory:
RIOTBASE ?= $(CURDIR)/../../..

# Comment this out to disable code in RIOT that does safety checking
# which is not needed in a production environment but helps in the
# development process:
DEVELHELP ?= 1

# Change this to 0 show compiler invocation lines by default:
QUIET ?= 1

WIFI_SSID ?= "lmyy"
WIFI_PASS ?= "lmylmylmy"
# WIFI_SSID ?= "Redmi Turbo 3"
# WIFI_PASS ?= "8dy4pet5udb7mf2"
# DEVICE NAME 也可自定义
CFLAGS += -DCONFIG_NIMBLE_AUTOADV_DEVICE_NAME='"NimBLE GATT Example"'
CFLAGS += -DCONFIG_NIMBLE_AUTOADV_START_MANUALLY=1

# use ESP WiFi in station mode as network interface
USEMODULE += esp_wifi
USEMODULE += cpp
USEMODULE += periph_i2c
USEMODULE += ztimer

# Optionally include remoteDNS support. This includes resolution of names at an
# upstream DNS server and the handling of RDNSS options in Router Advertisements
# to auto-configure that upstream DNS server.
# USEMODULE += sock_dns              # include DNS client
# USEMODULE += gnrc_ipv6_nib_dns     # include RDNSS option handling

ifneq (,$(DEFAULT_MQTT_CLIENT_ID))
  CFLAGS += -DDEFAULT_MQTT_CLIENT_ID=\"$(DEFAULT_MQTT_CLIENT_ID)\"
endif
ifneq (,$(DEFAULT_MQTT_USER))
  CFLAGS += -DDEFAULT_MQTT_USER=\"$(DEFAULT_MQTT_USER)\"
endif
ifneq (,$(DEFAULT_MQTT_PWD))
  CFLAGS += -DDEFAULT_MQTT_PWD=\"$(DEFAULT_MQTT_PWD)\"
endif

USEMODULE += shell_cmds_default
USEMODULE += ps
USEMODULE += netdev_default
USEPKG += paho-mqtt
USEMODULE += shell_cmd_i2c_scan

# paho-mqtt depends on TCP support, choose which stacks you want
GNRC_IPV6 ?= 0
LWIP_IPV4 ?= 1
LWIP_IPV6 ?= 0

include Makefile.lwip
include Makefile.gnrc

USEMODULE += sock_async_event
USEMODULE += sock_ip
USEMODULE += sock_udp
USEMODULE += sock_tcp

USEMODULE += ztimer
USEMODULE += ztimer_msec
USEMODULE += xtimer

####

include $(RIOTBASE)/Makefile.include

ifneq (,$(filter arch_esp,$(FEATURES_USED)))
  CFLAGS += -DWIFI_SSID=\"$(WIFI_SSID)\"
  CFLAGS += -DWIFI_PASS=\"$(WIFI_PASS)\"
endif
