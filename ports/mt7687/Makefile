# Select the board to build for: if not given on the command line, then default to MT7687LITE
BOARD ?= MT7687Lite
ifeq ($(wildcard boards/$(BOARD)/.),)
$(error Invalid BOARD specified)
endif

# If the build directory is not given, make it reflect the board name.
BUILD ?= build-$(BOARD)


include ../../py/mkenv.mk


CROSS_COMPILE ?= arm-none-eabi-

CFLAGS_CORTEX_M4 = -mthumb -march=armv7e-m -mabi=aapcs -mcpu=cortex-m4 -msoft-float -mfloat-abi=soft -fsingle-precision-constant -Wdouble-promotion
CFLAGS = -Wall -Wpointer-arith -Werror -std=gnu99 -nostdlib $(CFLAGS_CORTEX_M4) -Os
CFLAGS += -g -ffunction-sections -fdata-sections -fno-common -fsigned-char -mno-unaligned-access
CFLAGS += $(CFLAGS_MOD)

LDFLAGS = -Wl,-nostdlib -Wl,--gc-sections -Wl,-Map=$@.map


# qstr definitions (must come before including py.mk)
QSTR_DEFS = qstrdefsport.h $(BUILD)/pins_qstr.h

include $(TOP)/py/py.mk


INC =  -I.
INC += -I$(TOP)
INC += -I$(TOP)/lib/cmsis/inc
INC += -Ichip/Include
INC += -Irtos
INC += -Irtos/Source/include
INC += -Irtos/Source/portable/ARM_CM4
INC += -Imods
INC += -Imisc
INC += -Iboards/$(BOARD)
INC += -I$(BUILD)
INC += -I$(BUILD)/genhdr

CPPDEFINES = -Dgcc -DUSE_FREERTOS


MAIN_SRC_C = \
	main.c \
	mphalport.c \
	mpthreadport.c \
	fatfs_port.c \

CHIP_SRC_C = $(addprefix chip/Source/,\
	system_MT7687.c \
	MT7687_GPIO.c \
	MT7687_UART.c \
    MT7687_TIMR.c \
    MT7687_PWM.c  \
    MT7687_Flash.c \
)

CHIP_SRC_S = $(addprefix chip/Source/,\
	gcc/startup_MT7687.s \
)

RTOS_SRC_C = $(addprefix rtos/Source/,\
	croutine.c \
	event_groups.c \
	list.c \
	queue.c \
	tasks.c \
	timers.c \
	portable/ARM_CM4/port.c \
    portable/MemMang/heap_4.c \
)

MODS_SRC_C = $(addprefix mods/,\
	moduos.c \
	modmachine.c \
    modutime.c \
    modubinascii.c \
	pybuart.c \
    pybflash.c \
    pybpin.c \
    pybtimer.c \
    pybpwm.c \
)
#
# 	modnetwork.c \
# 	modusocket.c \
# 	modussl.c \
# 	modwlan.c \
# 	pybadc.c \
#
# 	pybi2c.c \
# 	pybrtc.c \
#
# 	pybsleep.c \
# 	pybspi.c \
#
#
# 	pybwdt.c \
#)

MISC_SRC_C = $(addprefix misc/,\
	fifo.c \
	bufhelper.c \
	gccollect.c \
	mperror.c \
	random.c \
	mpirq.c \
)
# 	ftp.c \
# 	updater.c \
# 	telnet.c \

MISC_SRC_S = $(addprefix misc/,\
	gchelper.s \
)

LIB_SRC_C = $(addprefix lib/,\
	oofatfs/ff.c \
	oofatfs/option/unicode.c \
	libc/string0.c \
	mp-readline/readline.c \
	netutils/netutils.c \
	timeutils/timeutils.c \
	utils/pyexec.c \
	utils/interrupt_char.c \
	utils/sys_stdio_mphal.c \
)

OBJ = $(PY_O) 
OBJ += $(addprefix $(BUILD)/, $(MAIN_SRC_C:.c=.o) $(CHIP_SRC_C:.c=.o) $(CHIP_SRC_S:.s=.o) $(RTOS_SRC_C:.c=.o))
OBJ += $(addprefix $(BUILD)/, $(MODS_SRC_C:.c=.o) $(MISC_SRC_C:.c=.o) $(MISC_SRC_S:.s=.o) $(LIB_SRC_C:.c=.o))
OBJ += $(BUILD)/pins.o

# List of sources for qstr extraction
SRC_QSTR += $(MODS_SRC_C) $(MISC_SRC_C)
# Append any auto-generated sources that are needed by sources listed in SRC_QSTR
SRC_QSTR_AUTO_DEPS +=

# Add the linker script
LINKER_SCRIPT = mt7687f.ld
LDFLAGS += -T $(LINKER_SCRIPT)

# Add the firmware specific CFLAGS
CFLAGS += $(CPPDEFINES) $(INC)



all: $(BUILD)/firmware.bin

$(BUILD)/firmware.elf: $(OBJ) $(LINKER_SCRIPT)
	$(ECHO) "LINK $@"
	$(Q)$(CC) -o $@ $(LDFLAGS) $(OBJ) $(LIBS)
	$(CROSS_COMPILE)objdump -d $@ > $(BUILD)/firmware.elf.dis
	$(Q)$(SIZE) $@

$(BUILD)/firmware.bin: $(BUILD)/firmware.elf
	$(ECHO) "Create $@"
	$(Q)$(OBJCOPY) -O binary $< $@


# Making OBJ use an order-only dependency on the generated pins.h file has the side 
# effect of making the pins.h file before we actually compile any of the objects. 
# The normal dependency generation will deal with the case when pins.h is modified. 
$(OBJ): | $(HEADER_BUILD)/pins.h

# Call make-pins.py to generate both pins.c and pins.h
$(BUILD)/pins.c $(HEADER_BUILD)/pins.h $(BUILD)/pins_qstr.h: chip/Include/MT7687_GPIO.h boards/MT7687_prefix.c | $(HEADER_BUILD)
	$(ECHO) "Create $@"
	$(Q)$(PYTHON) boards/make-pins.py --af chip/Include/MT7687_GPIO.h --prefix boards/MT7687_prefix.c --hdr $(HEADER_BUILD)/pins.h --qstr $(BUILD)/pins_qstr.h > $(BUILD)/pins.c

$(BUILD)/pins.o: $(BUILD)/pins.c
	$(call compile_c)


# always include MicroPython make rules
include $(TOP)/py/mkrules.mk
