APPLICATION = test_sw_2

# change accordingly to the location of your RIOT
RIOTBASE = /home/kyaso/src/RIOT

include $(RIOTBASE)/tests/Makefile.tests_common

BOARD = native

DISABLE_MODULE += auto_init

USEMODULE += od
USEMODULE += shell
USEMODULE += shell_commands
USEMODULE += ps


USEMODULE += treufunk

CFLAGS += -DDEVELHELP

include $(RIOTBASE)/Makefile.include
