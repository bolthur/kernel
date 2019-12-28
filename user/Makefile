
SRC = $(shell find . -name '*.c')
BIN = $(addsuffix .bin,$(basename $(SRC)))
DISASM = $(addsuffix .disasm,$(basename $(SRC)))

CC = /opt/bolthur/sysroot/arm/bin/arm-bolthur-eabi-gcc
OBJDUMP = /opt/bolthur/sysroot/arm/bin/arm-bolthur-eabi-objdump

ASFLAGS =
CFLAGS = -Wall -g -march=armv7-a -mtune=cortex-a7 -mfpu=neon-vfpv4 -mfloat-abi=hard
LDFLAGS =

all: $(BIN) $(DISASM)

%.bin: %.c
	$(CC) $(CFLAGS) $(LDFLAGS) $^ -o $@

%.disasm: %.bin
	$(OBJDUMP) -dx $^ > $@

clean:
	rm $(BIN)
	rm $(DISASM)

.PHONY: all clean
