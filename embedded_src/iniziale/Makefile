.SUFFIX:

TAFFO = taffo
TARGET ?= ARM

CFLAGS_ARM += --target=arm-none-eabi -mcpu=cortex-m3 --sysroot=/lib/arm-none-eabi -mthumb -ffreestanding -nostdlib -fshort-enums -c

TAFFO_FLAGS = -temp-dir taffo-temp

OUTOBJ_ = hand_iniziale
OUTOBJ_ARM = $(OUTOBJ_).o

C_SRC += $(wildcard *.cpp)
#C_SRC += $(wildcard *.c)

.PHONY: clean all

all: taffo-temp
	@echo Compiling for target: $(TARGET)
	taffo $(CFLAGS_$(TARGET)) $(TAFFO_FLAGS) $(C_SRC) -o $(OUTOBJ_$(TARGET))

taffo-temp:
	mkdir -p $@

clean:
	rm -f *.o
	rm -f *.ppm
	rm -rf taffo-temp

