
SRC = $(shell find . -name '*.c')
BIN = $(addsuffix .bin,$(basename $(SRC)))
DISASM = $(addsuffix .disasm,$(basename $(SRC)))

CC = /opt/bolthur/cross/bin/arm-unknown-bolthur-eabi-gcc
OBJDUMP = /opt/bolthur/cross/bin/arm-unknown-bolthur-eabi-objdump

ASFLAGS =
CFLAGS = -Wall -g -march=armv7-a -mtune=cortex-a7 -mfpu=neon-vfpv4 -mfloat-abi=hard
LDFLAGS =

TEST_CFLAGS = -march=armv7-a -mtune=cortex-a7 -mfpu=neon-vfpv4 -mfloat-abi=hard  -Wall -Wextra -Werror -Wpedantic -Wconversion -Wpacked -Wpacked-bitfield-compat -Wpacked-not-aligned -ffreestanding -fno-exceptions -nodefaultlibs -std=c18 -fomit-frame-pointer -g -O2 -nostdlib
TEST_LDFLAGS = -T link.ld

all: $(BIN) $(DISASM)

test.bin: test.c
	$(CC) $(TEST_CFLAGS) $(TEST_LDFLAGS) $^ -o $@

%.bin: %.c
	$(CC) $(CFLAGS) $(LDFLAGS) $^ -o $@

%.disasm: %.bin
	$(OBJDUMP) -dx $^ > $@

clean:
	rm $(BIN)
	rm $(DISASM)

.PHONY: all clean
