# Select the board to build for: if not given on the command line, then default to M482Lite
BOARD ?= M482Lite
ifeq ($(wildcard boards/$(BOARD)/.),)
$(error Invalid BOARD specified)
endif

# If the build directory is not given, make it reflect the board name.
BUILD ?= build-$(BOARD)


include ../../py/mkenv.mk


# qstr definitions (must come before including py.mk)
QSTR_DEFS = qstrdefsport.h $(BUILD)/pins_qstr.h

include $(TOP)/py/py.mk


CROSS_COMPILE ?= arm-none-eabi-


CFLAGS  = -mthumb -mcpu=cortex-m4 -mfloat-abi=soft -fsingle-precision-constant -Wdouble-promotion
CFLAGS += -Wall -Wpointer-arith -std=gnu99 -nostdlib
CFLAGS += -ffunction-sections -fdata-sections -fno-common -fsigned-char -mno-unaligned-access
CFLAGS += $(CFLAGS_MOD)

DEBUG = 1
ifeq ($(DEBUG),1)
CFLAGS += -O0 -g
else
CFLAGS += -O2
endif

LDFLAGS = -Wl,-nostdlib -Wl,--gc-sections -Wl,-Map=$@.map -T m480.ld -mthumb -mcpu=cortex-m4 -mfloat-abi=soft


INC =  -I.
INC += -I$(TOP)
INC += -I$(TOP)/lib/cmsis/inc
INC += -Ichip
INC += -Ichip/regs
INC += -Imods
INC += -Imisc
INC += -Irtos
INC += -Irtos/include
INC += -Irtos/ports/ARM_CM4
INC += -Iboards/$(BOARD)
INC += -I$(BUILD)
INC += -I$(BUILD)/genhdr

CFLAGS += $(INC)


MAIN_SRC_C = \
	main.c \
    misc.c \
	mphalport.c \

MODS_SRC_C = $(addprefix mods/,\
	moduos.c \
	modutime.c \
	modmachine.c \
	pybpin.c \
	pybuart.c \
	pybflash.c \
	pybtimer.c \
	pybspi.c \
	pybi2c.c \
	pybadc.c \
	pybdac.c \
    pybpwm.c \
    pybrtc.c \
    pybrng.c \
    pybusb.c \
    pybusb_desc.c \
    pybusb_vcom.c \
)

MISC_SRC_C = $(addprefix misc/,\
	bufhelper.c \
	gccollect.c \
)

CHIP_SRC_C = $(addprefix chip/,\
	system_M480.c \
	acmp.c \
	bpwm.c \
	can.c \
	clk.c \
	crc.c \
	crypto.c \
	dac.c \
	eadc.c \
	ebi.c \
	ecap.c \
	emac.c \
	epwm.c \
	fmc.c \
	gpio.c \
	hsusbd.c \
	i2c.c \
	i2s.c \
	pdma.c \
	qei.c \
	qspi.c \
	rtc.c \
	sc.c \
	scuart.c \
	sdh.c \
	spi.c \
	spim.c \
	sys.c \
	timer.c \
	timer_pwm.c \
	uart.c \
	usbd.c \
	usci_i2c.c \
	usci_spi.c \
	usci_uart.c \
	wdt.c \
	wwdt.c \
)

CHIP_SRC_S = $(addprefix chip/,\
	boot/gcc/startup_M480.s \
)

RTOS_SRC_C = $(addprefix rtos/,\
	list.c \
	queue.c \
	tasks.c \
	timers.c \
	ports/ARM_CM4/port.c \
	ports/MemMang/heap_4.c \
)

LIBS_SRC_C = $(addprefix lib/,\
	oofatfs/ff.c \
	oofatfs/ffunicode.c \
	libc/string0.c \
	mp-readline/readline.c \
	timeutils/timeutils.c \
	utils/pyexec.c \
	utils/interrupt_char.c \
	utils/sys_stdio_mphal.c \
)

OBJ = $(PY_O) 
OBJ += $(addprefix $(BUILD)/, $(MAIN_SRC_C:.c=.o) $(MODS_SRC_C:.c=.o) $(MISC_SRC_C:.c=.o) )
OBJ += $(addprefix $(BUILD)/, $(CHIP_SRC_C:.c=.o) $(CHIP_SRC_S:.s=.o) $(RTOS_SRC_C:.c=.o) $(LIBS_SRC_C:.c=.o))
OBJ += $(BUILD)/pins.o


# List of sources for qstr extraction
SRC_QSTR += $(MODS_SRC_C) $(MISC_SRC_C)
# Append any auto-generated sources that are needed by sources listed in SRC_QSTR
SRC_QSTR_AUTO_DEPS +=



all: $(BUILD)/micropython.bin

$(BUILD)/micropython.elf: $(OBJ)
	$(ECHO) "LINK $@"
	$(Q)$(CC) -o $@ $(LDFLAGS) $(OBJ) $(LIBS)
	$(CROSS_COMPILE)objdump -d $@ > $(BUILD)/micropython.elf.dis
	$(Q)$(SIZE) $@

$(BUILD)/micropython.bin: $(BUILD)/micropython.elf
	$(ECHO) "Create $@"
	$(Q)$(OBJCOPY) -O binary $< $@


# Making OBJ use an order-only dependency on the generated pins.h file has the side 
# effect of making the pins.h file before we actually compile any of the objects. 
# The normal dependency generation will deal with the case when pins.h is modified. 
$(OBJ): | $(HEADER_BUILD)/pins.h

# Call make-pins.py to generate both pins.c and pins.h
$(BUILD)/pins.c $(HEADER_BUILD)/pins.h $(BUILD)/pins_qstr.h: chip/sys.h boards/M480_prefix.c | $(HEADER_BUILD)
	$(ECHO) "Create $@"
	$(Q)$(PYTHON) boards/make-pins.py --af chip/sys.h --prefix boards/M480_prefix.c --hdr $(HEADER_BUILD)/pins.h --qstr $(BUILD)/pins_qstr.h --af-const $(HEADER_BUILD)/pins_af_const.h > $(BUILD)/pins.c

$(BUILD)/pins.o: $(BUILD)/pins.c
	$(call compile_c)


# always include MicroPython make rules
include $(TOP)/py/mkrules.mk
