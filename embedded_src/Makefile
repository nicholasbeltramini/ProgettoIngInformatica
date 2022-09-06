.SUFFIXES: # builtin rules are EVIL
CC = arm-none-eabi-gcc
CXX = arm-none-eabi-g++
AS = arm-none-eabi-as
OBJCOPY = arm-none-eabi-objcopy

OUTPUT = embedded_main
TARGET ?= stm32f207

OBJ_DIR = ./obj
OUTPUT_ELF = $(OUTPUT).elf
OUTPUT_BIN = $(OUTPUT).bin
OUTPUT_MAP = $(OUTPUT).map

# variabile inserita per richiamare la shell di bash
SHELL = bash

.PHONY: all clean annotato iniziale

all: annotato iniziale $(OUTPUT_BIN)

C_SRC += $(wildcard *.c)

CFLAGS = -ffreestanding -g
ASMFLAGS = -ffreestanding -g
LDFLAGS = -ffreestanding -lm -lc -Wl,-Map=$(OUTPUT_MAP)
-include $(TARGET)/Makefile.inc

C_OBJ = $(patsubst %, $(OBJ_DIR)/%, $(notdir $(C_SRC:.c=.o)))
ASM_OBJ = $(patsubst %, $(OBJ_DIR)/%, $(notdir $(ASM_SRC:.s=.o)))
OBJ = $(C_OBJ) $(ASM_OBJ) $(wildcard bench_obj/*.o)
DEPS = $(C_OBJ:.o=.d)

-include $(DEPS)

$(OBJ_DIR)/%.o: %.c
	$(CC) $(CFLAGS) -MMD -c $< -o $@

$(OBJ_DIR)/%.o: $(TARGET)/%.c
	$(CC) $(CFLAGS) -MMD -c $< -o $@

$(OBJ_DIR)/%.o: %.s
	/opt/llvm-12.0-d/bin/clang++ --target=arm-none-eabi $(ASMFLAGS) -MMD -c $< -o $@

$(OBJ_DIR)/%.o: $(TARGET)/%.s
	$(CXX) $(ASMFLAGS) -MMD -c $< -o $@

$(OUTPUT_ELF): $(OBJ)
	$(CXX) $(LDFLAGS) $^ -o $@

%.bin: %.elf
	$(OBJCOPY) -O binary $< $@
	
iniziale:
	cd iniziale && make && cp hand_iniziale.o ../bench_obj/

annotato:
	cd annotato && make && cp hand_annotato.o ../bench_obj/

$(OBJ): | $(OBJ_DIR)

$(OBJ_DIR):
	mkdir -p $@

clean:
	rm -rf $(OBJ_DIR)
	rm -f *.elf *.bin *.map *.log *.s
	rm -f bench_obj/hand_iniziale.o
	rm -f bench_obj/hand_annotato.o
	cd annotato && make clean
	cd iniziale && make clean

