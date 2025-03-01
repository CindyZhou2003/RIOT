# Ensure minimal size by default
DEVELHELP ?= 0
APPLICATION = tingml
BOARD ?= esp32-wroom-32

USEPKG += tflite-micro

# TensorFlow-Lite crashes on M4/M7 CPUs when FPU is enabled, so disable it by
# default for now
DISABLE_MODULE += cortexm_fpu
USEMODULE += gesture
FEATURES_REQUIRED += cpp
FEATURES_REQUIRED += periph_i2c
FEATURES_OPTIONAL += periph_i2c_reconfigure
EXTERNAL_MODULE_DIRS += external_modules


USEMODULE += shell_cmds_default
USEMODULE += shell_cmd_i2c_scan
USEMODULE += xtimer
USEMODULE += ztimer
RIOTBASE ?= $(CURDIR)/../../..
# As there is an 'Kconfig' we want to explicitly disable Kconfig by setting
# the variable to empty
SHOULD_RUN_KCONFIG ?=

include $(RIOTBASE)/Makefile.include
