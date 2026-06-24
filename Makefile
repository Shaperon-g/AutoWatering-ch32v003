TARGET = AutoWatering

UPDATE_TIME = 12 #hours
WATERING_DURATION = 2000 #ms

OPT = -O2
FORGDB = -g

LDSCRIPT = V0_Link.ld
OCDSCRIPT = wch-riscv.cfg

BUILD_DIR = build/
SRC_DIR = src/
####### source files ######
C_SOURCES = src/core/core_riscv.c
ASM_SOURCES = src/core/startup_ch32v00x.S
CPP_SOURCES = src/main.cpp \
              src/GPIO.cpp \
			  src/uart/uart.cpp

VPATH = src \
        src/core \
		src/uart

OBJECTS := $(addprefix $(BUILD_DIR)/, $(notdir $(C_SOURCES:.c=.o)))
OBJECTS += $(addprefix $(BUILD_DIR)/, $(notdir $(CPP_SOURCES:.cpp=.o)))
OBJECTS += $(addprefix $(BUILD_DIR)/, $(notdir $(ASM_SOURCES:.S=.o)))

####### WCH gcc ######
#if it isn`t in system path:
RISCV_DIR = 
OPENOCD_DIR = 

CC = riscv-wch-elf-gcc
AS = riscv-wch-elf-gcc -x assembler-with-cpp
CP = riscv-wch-elf-objcopy
SZ = riscv-wch-elf-size

OCD = openocd

####### flags ######
HEX = $(CP) -O ihex
BIN = $(CP) -O binary -S

# For gcc v12 and above (else: -march=rv32imac)
# CPU = -march=rv32imac_zicsr -mabi=ilp32 -msmall-data-limit=8
# v003 : CPU = -march=rv32ec_zicsr -mabi=ilp32e -msmall-data-limit=8
# v203 : CPU = -march=rv32imac_zicsr -mabi=ilp32e -msmall-data-limit=8
CPU = -march=rv32ec_zicsr -mabi=ilp32e -msmall-data-limit=8
CPU_LD = -march=rv32ec -mabi=ilp32e -msmall-data-limit=8

AS_INCLUDES =
C_INCLUDES = $(addprefix -I, $(VPATH))

ASFLAGS = $(CPU) $(AS_INCLUDES) $(OPT) $(FORGDB) -Wall -fdata-sections -ffunction-sections
CFLAGS = $(CPU) $(C_INCLUDES) $(OPT) $(FORGDB) -Wall -fdata-sections -ffunction-sections
CFLAGS += -MMD -MP -MF"$(@:%.o=%.d)"
CFLAGS += -DAWTR_UPDATE_TIME=$(UPDATE_TIME) -DAWTR_WATERING_DUR=$(WATERING_DURATION)


LIBS = -lc -lm -lnosys #-nostdlib
LDFLAGS = $(CPU_LD) -mno-save-restore -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections \
   -Wunused -Wuninitialized -T $(LDSCRIPT) -nostartfiles -Xlinker --gc-sections -Wl,-Map=$(BUILD_DIR)/$(TARGET).map \
   --specs=nano.specs $(LIBS)

####################################################################################################
all: $(BUILD_DIR)/$(TARGET).elf $(BUILD_DIR)/$(TARGET).hex $(BUILD_DIR)/$(TARGET).bin


$(BUILD_DIR)/%.o: %.c %.h Makefile | $(BUILD_DIR)
	$(CC) -c $(CFLAGS) -Wa,-a,-ad,-alms=$(BUILD_DIR)/$(notdir $(<:.c=.lst)) $< -o $@

$(BUILD_DIR)/%.o: %.cpp Makefile | $(BUILD_DIR)
	$(CC) -c $(CFLAGS) -Wa,-a,-ad,-alms=$(BUILD_DIR)/$(notdir $(<:.cpp=.lst)) $< -o $@

$(BUILD_DIR)/%.o: %.S Makefile | $(BUILD_DIR)
	$(AS) -c $(ASFLAGS) $< -o $@

$(BUILD_DIR)/$(TARGET).elf: $(OBJECTS) Makefile
	$(CC) $(OBJECTS) $(LDFLAGS) -o $@
	$(SZ) $@

$(BUILD_DIR)/%.hex: $(BUILD_DIR)/%.elf | $(BUILD_DIR)
	@$(HEX) $< $@

$(BUILD_DIR)/%.bin: $(BUILD_DIR)/%.elf | $(BUILD_DIR)
	@$(BIN) $< $@

$(BUILD_DIR):
	mkdir $@


####################################################################################################
.phony: load clean test

load: $(BUILD_DIR)/$(TARGET).elf $(OBJECTS)
	$(OCD) -f $(OCDSCRIPT) -c 'init; halt; program $(BUILD_DIR)/$(TARGET).hex verify; reset; wlink_reset_resume; exit;'

clean:
	@rm -rf $(BUILD_DIR)

test:
	@echo C L$(C_SOURCES)O ahahaha
	@echo CPP L$(CPP_SOURCES)O
	@echo ASM L$(ASM_SOURCES)O
	@echo OBJ L$(OBJECTS)O

####################################################################################################
# dependencies
-include $(wildcard $(BUILD_DIR)/*.d)

