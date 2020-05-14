# Select the board to build for: if not given on the command line, then default to M482Lite
BOARD ?= M482Lite
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
LDFLAGS += -Wl,--gc-sections -Wl,-nostdlib -Wl,-Map=$@.map -T m480.ld
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
	-Ichip/regs \
	-Imods \
	-Imisc \
	-Iboards/$(BOARD) \
	-I$(BUILD) \
	-I$(BUILD)/genhdr \

CFLAGS += $(INCS)


C_APP = \
	main.c \
	mphalport.c \

C_MODS = $(addprefix mods/, \
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
    pybcan.c \
	pybrtc.c \
    pybrng.c \
    pybwdt.c \
	pybusb.c \
	pybusb_desc.c \
	pybusb_vcom.c \
)

C_MISC = $(addprefix misc/, \
	bufhelper.c \
	gccollect.c \
)

C_CHIP = $(addprefix chip/, \
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

S_CHIP = $(addprefix chip/startup/gcc/, \
    startup_M480.s \
)

C_LIB = $(addprefix lib/, \
	libc/string0.c \
	utils/pyexec.c \
	mp-readline/readline.c \
	utils/interrupt_char.c \
	utils/sys_stdio_mphal.c \
	timeutils/timeutils.c \
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


OBJ = $(PY_O) 
OBJ += $(addprefix $(BUILD)/, $(C_APP:.c=.o) $(C_CHIP:.c=.o) $(S_CHIP:.s=.o) $(C_MODS:.c=.o) $(C_MISC:.c=.o))
OBJ += $(addprefix $(BUILD)/, $(C_LIB:.c=.o) $(C_LIBM:.c=.o) $(SRC_MOD:.c=.o))
OBJ += $(BUILD)/pins.o


# List of sources for qstr extraction
SRC_QSTR += $(C_MODS) $(C_MISC)
# Append any auto-generated sources that are needed by sources listed in SRC_QSTR
SRC_QSTR_AUTO_DEPS +=



all: $(BUILD)/micropython.bin

$(BUILD)/micropython.elf: $(OBJ)
	$(ECHO) "LINK $@"
	$(Q)$(CC) -o $@ $(LDFLAGS) $(OBJ)
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
