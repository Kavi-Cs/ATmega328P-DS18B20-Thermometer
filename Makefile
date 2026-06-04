# Hey Emacs, this is a -*- makefile -*-
#----------------------------------------------------------------------------
# WinAVR Makefile Template written by Eric B. Weddington, J�rg Wunsch, et al.
#
# Released to the Public Domain
#
# Additional material for this makefile was written by:
# Peter Fleury
# Tim Henigan
# Colin O'Flynn
# Reiner Patommel
# Markus Pfaff
# Sander Pool
# Frederik Rouleau
# Carlos Lamas
#
#----------------------------------------------------------------------------
# On command line:
#
# make all = Make software.
#
# make clean = Clean out built project files.
#
# make coff = Convert ELF to AVR COFF.
#
# make extcoff = Convert ELF to AVR Extended COFF.
#
# make program = Download the hex file to the device, using avrdude.
#                Please customize the avrdude settings below first!
#
# make debug = Start either simulavr or avarice as specified for debugging, 
#              with avr-gdb or avr-insight as the front end for debugging.
#
# make filename.s = Just compile filename.c into the assembler code only.
#
# make filename.i = Create a preprocessed source file for use in submitting
#                   bug reports to the GCC project.
#
# To rebuild project do "make clean" then "make all".
#----------------------------------------------------------------------------


# MCU name
MCU = atmega328p

# Processor frequency.
F_CPU = 1000000

# Target file name (without extension).
TARGET = main

# Object files directory
OBJDIR = .

# List C source files here.
SRC = $(TARGET).c

# List C++ source files here.
CPPSRC = 

# List Assembler source files here.
ASRC =

# Optimization level
OPT = s

# Debugging format.
DEBUG = dwarf-2

# C Standard level.
CSTANDARD = -std=gnu99

#---------------- Programming Options (avrdude) ----------------
AVRDUDE_PROGRAMMER = usbasp
AVRDUDE_PORT = usb
AVRDUDE_BITCLOCK = 125

# USBIPD settings for WSL 2 (use 'usbipd list' in Windows to find busid)
USBIPD_BUSID = 1-9

# Fuse settings for ATmega328p (Default: Internal 8MHz clock / 8 = 1MHz)
LFUSE = 0x62
HFUSE = 0xD9
EFUSE = 0xFF

AVRDUDE_FLAGS = -p $(MCU) -P $(AVRDUDE_PORT) -c $(AVRDUDE_PROGRAMMER) -B $(AVRDUDE_BITCLOCK)

#---------------- Compiler Options ----------------
CFLAGS = -g$(DEBUG)
CFLAGS += -DF_CPU=$(F_CPU)UL
CFLAGS += -O$(OPT)
CFLAGS += -funsigned-char -funsigned-bitfields -fpack-struct -fshort-enums
CFLAGS += -Wall -Wstrict-prototypes
CFLAGS += $(CSTANDARD)
CFLAGS += -MMD -MP -MF .dep/$(@F).d

#---------------- Linker Options ----------------
LDFLAGS = -Wl,-Map=$(TARGET).map,--cref
LDFLAGS += -lm

#---------------- Programs ----------------
CC = avr-gcc
OBJCOPY = avr-objcopy
OBJDUMP = avr-objdump
SIZE = avr-size
NM = avr-nm
AVRDUDE = avrdude
REMOVE = rm -f
REMOVEDIR = rm -rf

# Define all object files.
OBJ = $(SRC:%.c=$(OBJDIR)/%.o) $(CPPSRC:%.cpp=$(OBJDIR)/%.o) $(ASRC:%.S=$(OBJDIR)/%.o) 

# Default target.
all: build size

build: elf hex eep lss sym

elf: $(TARGET).elf
hex: $(TARGET).hex
eep: $(TARGET).eep
lss: $(TARGET).lss
sym: $(TARGET).sym

%.elf: $(OBJ)
	@echo "Linking: $@"
	$(CC) -mmcu=$(MCU) $(LDFLAGS) $^ -o $@

$(OBJDIR)/%.o: %.c | .dep
	@echo "Compiling: $<"
	$(CC) -c -mmcu=$(MCU) $(CFLAGS) -Wa,-adhlns=$(<:%.c=$(OBJDIR)/%.lst) $< -o $@

.dep:
	mkdir -p .dep

%.hex: %.elf
	@echo "Creating Flash Hex: $@"
	$(OBJCOPY) -O ihex -R .eeprom -R .fuse -R .lock $< $@

%.eep: %.elf
	@echo "Creating EEPROM Hex: $@"
	-$(OBJCOPY) -j .eeprom --set-section-flags=.eeprom="alloc,load" \
	--change-section-lma .eeprom=0 --no-change-warnings -O ihex $< $@ || exit 0

%.lss: %.elf
	@echo "Creating Listing: $@"
	$(OBJDUMP) -h -S -z $< > $@

%.sym: %.elf
	@echo "Creating Symbol Table: $@"
	$(NM) -n $< > $@

size: $(TARGET).elf
	@echo "Size after build:"
	$(SIZE) --mcu=$(MCU) --format=avr $(TARGET).elf

# Program the device.
program: $(TARGET).hex
	@echo "Ensuring USB device is attached..."
	-usbipd.exe attach --wsl --busid $(USBIPD_BUSID) 2>/dev/null || true
	sudo $(AVRDUDE) $(AVRDUDE_FLAGS) -U flash:w:$(TARGET).hex

flash: program

fuses:
	@echo "Setting fuses..."
	-usbipd.exe attach --wsl --busid $(USBIPD_BUSID) 2>/dev/null || true
	sudo $(AVRDUDE) $(AVRDUDE_FLAGS) -U lfuse:w:$(LFUSE):m -U hfuse:w:$(HFUSE):m -U efuse:w:$(EFUSE):m

clean:
	@echo "Cleaning project..."
	$(REMOVE) $(TARGET).hex $(TARGET).eep $(TARGET).elf $(TARGET).map $(TARGET).sym $(TARGET).lss
	$(REMOVE) $(OBJ)
	$(REMOVE) $(SRC:%.c=$(OBJDIR)/%.lst)
	$(REMOVEDIR) .dep

.PHONY: all build elf hex eep lss sym size program flash fuses clean
