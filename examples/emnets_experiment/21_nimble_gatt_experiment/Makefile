# Ensure minimal size by default
DEVELHELP ?= 0
# name of your application
APPLICATION = nimble_gatt
# If no BOARD is found in the environment, use this default:
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
USEMODULE += shell_cmds_default
USEMODULE += cpp
USEMODULE += ztimer
USEMODULE += xtimer
USEMODULE += shell_cmd_i2c_scan
# Use automated advertising
USEMODULE += nimble_autoadv
# DEVICE NAME 也可自定义
CFLAGS += -DCONFIG_NIMBLE_AUTOADV_DEVICE_NAME='"NimBLE GATT Example"'
CFLAGS += -DCONFIG_NIMBLE_AUTOADV_START_MANUALLY=1

# This has to be the absolute path to the RIOT base directory:
RIOTBASE ?= $(CURDIR)/../../..

SHOULD_RUN_KCONFIG ?=
include $(RIOTBASE)/Makefile.include
