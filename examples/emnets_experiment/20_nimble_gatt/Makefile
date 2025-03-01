# name of your application
APPLICATION = nimble_gatt

# If no BOARD is found in the environment, use this default:
BOARD ?= esp32-wroom-32

# This has to be the absolute path to the RIOT base directory:
RIOTBASE ?= $(CURDIR)/../../..

# Include NimBLE
FEATURES_REQUIRED += periph_i2c
FEATURES_OPTIONAL += periph_i2c_reconfigure

USEPKG += nimble
USEMODULE += nimble_svc_gap
USEMODULE += nimble_svc_gatt
USEMODULE += shell_cmds_default
USEMODULE += cpp
USEMODULE += ztimer
USEMODULE += xtimer
USEMODULE += shell_cmd_i2c_scan

# Use automated advertising
USEMODULE += nimble_autoadv
CFLAGS += -DCONFIG_NIMBLE_AUTOADV_DEVICE_NAME='"NimBLE GATT Example"'
CFLAGS += -DCONFIG_NIMBLE_AUTOADV_START_MANUALLY=1

# Comment this out to disable code in RIOT that does safety checking
# which is not needed in a production environment but helps in the
# development process:
DEVELHELP ?= 1

# Change this to 0 show compiler invocation lines by default:
QUIET ?= 1

include $(RIOTBASE)/Makefile.include
