APPLICATION = tf_hw_test

BOARD ?= arduino-due

DISABLE_MODULE += auto_init

#USEMODULE += od
#USEMODULE += xtimer
USEMODULE += shell
USEMODULE += shell_commands
#USEMODULE += ps

DRIVER := treufunk

# include the selected driver
USEMODULE += $(DRIVER)

CFLAGS += -DDEVELHELP

# Change this at IAS/home
# #include /home/kyaso/src/RIOT/Makefile.include
include /home/kyasotharan/riot/RIOT/Makefile.include
