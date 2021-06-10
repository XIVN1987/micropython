# Select the board to build for: if not given on the command line, then default to MT7687LITE
BOARD ?= MT7687Lite
ifeq ($(wildcard boards/$(BOARD)/.),)
$(error Invalid BOARD specified)
endif

# If the build directory is not given, make it reflect the board name.
BUILD ?= build-$(BOARD)


CROSS_COMPILE ?= arm-none-eabi-
include ../../py/mkenv.mk


CFLAGS  = -mthumb -mcpu=cortex-m4 -msoft-float
CFLAGS += -ffunction-sections -fdata-sections -funsigned-char
CFLAGS += $(CFLAGS_MOD)

ifndef NDEBUG
CFLAGS += -O0 -g
else
CFLAGS += -O2 -DNDEBUG
endif

LDFLAGS  = -mthumb -mcpu=cortex-m4 -msoft-float
LDFLAGS += -Wl,--gc-sections -Wl,-nostdlib -Wl,-Map=$@.map -T mt7687f.ld
LDFLAGS += $(LDFLAGS_MOD) -DUSE_FREERTOS


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
	-Irtos \
	-Irtos/include \
	-Irtos/portable/ARM_CM4 \
	-Iboards/$(BOARD) \
	-I$(BUILD) \
	-I$(BUILD)/genhdr \

CFLAGS += $(INCS)

C_APP = \
	main.c \
	mphalport.c \
	mpthreadport.c \
	fatfs_port.c \

C_CHIP = $(addprefix chip/, \
	system_MT7687.c \
	MT7687_GPIO.c \
	MT7687_UART.c \
	MT7687_TIMR.c \
	MT7687_Flash.c \
    MT7687_SPI.c \
    MT7687_I2C.c \
    MT7687_ADC.c \
    MT7687_PWM.c \
    MT7687_RTC.c \
    MT7687_WDT.c \
)

S_CHIP = $(addprefix chip/startup/gcc/, \
    startup_MT7687.s \
)

C_MODS = $(addprefix mods/, \
	moduos.c \
	modmachine.c \
	modutime.c \
	pybuart.c \
	pybflash.c \
	pybpin.c \
	pybtimer.c \
    pybspi.c \
    pybi2c.c \
    pybadc.c \
	pybpwm.c \
    pybrtc.c \
    pybwdt.c \
)
# 	modnetwork.c \
# 	modusocket.c \
# 	modussl.c \
# 	modwlan.c \

C_MISC = $(addprefix misc/, \
	fifo.c \
	bufhelper.c \
	gccollect.c \
	mperror.c \
	random.c \
)
# 	ftp.c \
# 	updater.c \
# 	telnet.c \

C_RTOS = $(addprefix rtos/, \
	croutine.c \
	event_groups.c \
	list.c \
	queue.c \
	tasks.c \
	timers.c \
	portable/ARM_CM4/port.c \
	portable/MemMang/heap_4.c \
)

C_LIB = $(addprefix lib/, \
	libc/string0.c \
	utils/pyexec.c \
	mp-readline/readline.c \
	utils/interrupt_char.c \
	utils/sys_stdio_mphal.c \
	netutils/netutils.c \
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
OBJ += $(addprefix $(BUILD)/, $(C_LIB:.c=.o) $(S_LIB:.s=.o)  $(C_LIBM:.c=.o) $(C_RTOS:.c=.o) $(SRC_MOD:.c=.o))
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
$(BUILD)/pins.c $(HEADER_BUILD)/pins.h $(BUILD)/pins_qstr.h: chip/MT7687_GPIO.h boards/MT7687_prefix.c | $(HEADER_BUILD)
	$(ECHO) "Create $@"
	$(Q)$(PYTHON) boards/make-pins.py --af chip/MT7687_GPIO.h --prefix boards/MT7687_prefix.c --hdr $(HEADER_BUILD)/pins.h --qstr $(BUILD)/pins_qstr.h > $(BUILD)/pins.c

$(BUILD)/pins.o: $(BUILD)/pins.c
	$(call compile_c)


# always include MicroPython make rules
include $(TOP)/py/mkrules.mk
