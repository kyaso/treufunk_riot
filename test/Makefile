APPLICATION = tf_sw_test

BOARD ?= native

DISABLE_MODULE += auto_init

USEMODULE += od
USEMODULE += xtimer
#USEMODULE += shell
#USEMODULE += shell_commands
#USEMODULE += ps

DRIVER := treufunk

# include the selected driver
USEMODULE += $(DRIVER)

CFLAGS += -DDEVELHELP

include /home/kyasotharan/riot/RIOT/Makefile.include
