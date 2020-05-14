# Select the board to build for: if not given on the command line, then default to SWM320Lite
BOARD ?= SWM320Lite
ifeq ($(wildcard boards/$(BOARD)/.),)
$(error Invalid BOARD specified)
endif

# If the build directory is not given, make it reflect the board name.
BUILD ?= build-$(BOARD)


CROSS_COMPILE ?= arm-none-eabi-
include ../../py/mkenv.mk


CFLAGS  = -mthumb -mcpu=cortex-m4 -mfloat-abi=soft
CFLAGS += -ffunction-sections -fdata-sections -funsigned-char
CFLAGS += $(CFLAGS_MOD)

ifndef NDEBUG
CFLAGS += -O0 -g
else
CFLAGS += -O2 -DNDEBUG
endif

LDFLAGS  = -mthumb -mcpu=cortex-m4 -mfloat-abi=soft
LDFLAGS += -Wl,--gc-sections -Wl,-nostdlib -Wl,-Map=$@.map -T swm320.ld
LDFLAGS += $(LDFLAGS_MOD)


# qstr definitions (must come before including py.mk)
QSTR_DEFS = qstrdefsport.h $(BUILD)/pins_qstr.h

MICROPY_VFS_FAT = 1

include $(TOP)/py/py.mk


INCS = \
	-I. \
	-I$(TOP) \
	-I$(TOP)/lib/cmsis/inc \
	-Ichip \
	-Imods \
	-Imisc \
	-Iboards/$(BOARD) \
	-I$(BUILD) \
	-I$(BUILD)/genhdr \

CFLAGS += $(INCS)

C_APP = \
	main.c \
	mphalport.c \

C_CHIP = $(addprefix chip/, \
	SWM320_adc.c \
	SWM320_can.c \
	SWM320_crc.c \
	SWM320_dma.c \
	SWM320_exti.c \
	SWM320_flash.c \
	SWM320_gpio.c \
	SWM320_i2c.c \
	SWM320_lcd.c \
	SWM320_port.c \
	SWM320_pwm.c \
	SWM320_rtc.c \
	SWM320_sdio.c \
	SWM320_spi.c \
	SWM320_timr.c \
	SWM320_uart.c \
	SWM320_wdt.c \
	system_SWM320.c \
)

S_CHIP = $(addprefix chip/startup/gcc/, \
	startup_SWM320.s \
)

C_MODS = $(addprefix mods/, \
	moduos.c \
	modutime.c \
	modmachine.c \
	pybpin.c \
	pybuart.c \
	pybflash.c \
    pybtimer.c \
    pybpulse.c \
	pybspi.c \
	pybi2c.c \
    pybadc.c \
	pybpwm.c \
    pybcan.c \
    pybrtc.c \
    pybwdt.c \
)

C_MISC = $(addprefix misc/, \
	bufhelper.c \
	gccollect.c \
	random.c \
)

C_LIB = $(addprefix lib/, \
	libc/string0.c \
	utils/pyexec.c \
    mp-readline/readline.c \
	utils/interrupt_char.c \
	utils/sys_stdio_mphal.c \
    timeutils/timeutils.c \
)

S_LIB = $(addprefix lib/, \
	utils/gchelper_m3.s \
)

C_LIBM = $(addprefix lib/libm/, \
	math.c \
	acoshf.c \
	asinfacosf.c \
	asinhf.c \
	atan2f.c \
	atanf.c \
	atanhf.c \
	ef_rem_pio2.c \
	erf_lgamma.c \
	fmodf.c \
	kf_cos.c \
	kf_rem_pio2.c \
	kf_sin.c \
	kf_tan.c \
	log1pf.c \
	nearbyintf.c \
	sf_cos.c \
	sf_erf.c \
	sf_frexp.c \
	sf_ldexp.c \
	sf_modf.c \
	sf_sin.c \
	sf_tan.c \
	wf_lgamma.c \
	wf_tgamma.c \
	ef_sqrt.c \
)

OBJ  = $(PY_O) 
OBJ += $(addprefix $(BUILD)/, $(C_APP:.c=.o) $(C_CHIP:.c=.o) $(S_CHIP:.s=.o) $(C_MODS:.c=.o) $(C_MISC:.c=.o))
OBJ += $(addprefix $(BUILD)/, $(C_LIB:.c=.o) $(S_LIB:.s=.o)  $(C_LIBM:.c=.o) $(SRC_MOD:.c=.o))
OBJ += $(BUILD)/pins.o

# List of sources for qstr extraction
SRC_QSTR += $(C_MODS) $(C_MISC)
# Append any auto-generated sources that are needed by sources listed in SRC_QSTR
SRC_QSTR_AUTO_DEPS +=


all: $(BUILD)/micropython.bin

$(BUILD)/micropython.elf: $(OBJ)
	$(ECHO) "LINK $@"
	$(Q)$(CC) -o $@ $(LDFLAGS) $(OBJ)
	$(CROSS_COMPILE)objdump -d $@ > $@.dis
	$(Q)$(SIZE) $@

$(BUILD)/micropython.bin: $(BUILD)/micropython.elf
	$(ECHO) "Create $@"
	$(Q)$(OBJCOPY) -O binary $< $@


# Making OBJ use an order-only dependency on the generated pins.h file has the side 
# effect of making the pins.h file before we actually compile any of the objects. 
# The normal dependency generation will deal with the case when pins.h is modified. 
$(OBJ): | $(HEADER_BUILD)/pins.h

# Call make-pins.py to generate both pins.c and pins.h
$(BUILD)/pins.c $(HEADER_BUILD)/pins.h $(BUILD)/pins_qstr.h: chip/SWM320_port.h boards/SWM320_prefix.c | $(HEADER_BUILD)
	$(ECHO) "Create $@"
	$(Q)$(PYTHON) boards/make-pins.py --af chip/SWM320_port.h --prefix boards/SWM320_prefix.c --hdr $(HEADER_BUILD)/pins.h --qstr $(BUILD)/pins_qstr.h > $(BUILD)/pins.c

$(BUILD)/pins.o: $(BUILD)/pins.c
	$(call compile_c)


# always include MicroPython make rules
include $(TOP)/py/mkrules.mk
