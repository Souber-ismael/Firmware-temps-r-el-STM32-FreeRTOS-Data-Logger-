# Tests unitaires hote : Unity + gcc (sans STM32 / FreeRTOS)
# Usage, depuis Tests/ :
#   make test_supervisor
#   make test_aht20
#   make all
#   make clean

CC      ?= gcc
CFLAGS  ?= -std=c99 -Wall -Wextra -Werror -g
ROOT     = ..
BUILD    = build

UNITY_SRC = unity/unity.c
INCLUDES  = -Iunity

.PHONY: all test_supervisor test_aht20 clean

all: test_supervisor test_aht20

$(BUILD):
	mkdir -p $(BUILD)

SUPERVISOR_SRCS = \
	$(UNITY_SRC) \
	$(ROOT)/APP/task_supervisor/supervisor_logic.c \
	test_supervisor/test_supervisor_logic.c

$(BUILD)/test_supervisor.exe: $(SUPERVISOR_SRCS) | $(BUILD)
	$(CC) $(CFLAGS) $(INCLUDES) -I$(ROOT)/APP/task_supervisor \
		$(SUPERVISOR_SRCS) -o $@

test_supervisor: $(BUILD)/test_supervisor.exe
	$(BUILD)/test_supervisor.exe

AHT20_SRCS = \
	$(UNITY_SRC) \
	$(ROOT)/Driver/AHT20/aht20_convert.c \
	test_aht20/test_aht20_convert.c

$(BUILD)/test_aht20.exe: $(AHT20_SRCS) | $(BUILD)
	$(CC) $(CFLAGS) $(INCLUDES) -I$(ROOT)/Driver/AHT20 \
		$(AHT20_SRCS) -o $@

test_aht20: $(BUILD)/test_aht20.exe
	$(BUILD)/test_aht20.exe

clean:
	rm -rf $(BUILD)
