# name of your application
APPLICATION = tingml_get_dataset

BOARD ?= esp32-wroom-32

# This has to be the absolute path to the RIOT base directory:
RIOTBASE ?= $(CURDIR)/../../..

# Comment this out to disable code in RIOT that does safety checking
# which is not needed in a production environment but helps in the
# development process:
DEVELHELP ?= 1
USEMODULE += shell_cmds_default
USEMODULE += ztimer
USEMODULE += xtimer
USEMODULE += cpp
USEMODULE += shell_cmd_i2c_scan
# Change this to 0 show compiler invocation lines by default:
QUIET ?= 1

# Use a peripheral timer for the delay, if available
FEATURES_OPTIONAL += periph_timer
FEATURES_REQUIRED += periph_i2c
FEATURES_OPTIONAL += periph_i2c_reconfigure
include $(RIOTBASE)/Makefile.include
