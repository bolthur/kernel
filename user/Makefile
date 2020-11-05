
SRC = $(shell find . -name '*.c')
BIN = $(addsuffix .bin,$(basename $(SRC)))
DISASM = $(addsuffix .disasm,$(basename $(SRC)))

CC = /opt/bolthur/cross/bin/arm-unknown-bolthur-eabi-gcc
OBJDUMP = /opt/bolthur/cross/bin/arm-unknown-bolthur-eabi-objdump

ASFLAGS =
CFLAGS = -Wall -g -march=armv7-a -mtune=cortex-a7 -mfpu=neon-vfpv4 -mfloat-abi=hard
LDFLAGS =

TEST_CFLAGS = -march=armv7-a -mtune=cortex-a7 -mfpu=neon-vfpv4 -mfloat-abi=hard  -Wall -Wextra -Werror -Wpedantic -Wconversion -Wpacked -Wpacked-bitfield-compat -Wpacked-not-aligned -Wno-sign-conversion -Wno-unused-variable -ffreestanding -fno-exceptions -nodefaultlibs -std=c18 -fomit-frame-pointer -g -O2 -nostdlib
TEST_LDFLAGS = -T link.ld

all: $(BIN) $(DISASM)

test_generic.bin: test_generic.c
	$(CC) $(TEST_CFLAGS) $(TEST_LDFLAGS) $^ -o $@

test_sys_alloc_dealloc.bin: test_sys_alloc_dealloc.c
	$(CC) $(TEST_CFLAGS) $(TEST_LDFLAGS) $^ -o $@

test_process_exit.bin: test_process_exit.c
	$(CC) $(TEST_CFLAGS) $(TEST_LDFLAGS) $^ -o $@

test_thread_exit.bin: test_thread_exit.c
	$(CC) $(TEST_CFLAGS) $(TEST_LDFLAGS) $^ -o $@

%.bin: %.c
	$(CC) $(CFLAGS) $(LDFLAGS) $^ -o $@

%.disasm: %.bin
	$(OBJDUMP) -dx $^ > $@

clean:
	rm $(BIN)
	rm $(DISASM)

.PHONY: all clean
