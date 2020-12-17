
SRC = $(shell find . -name '*.c')
BIN = $(addsuffix .bin,$(basename $(SRC)))
SYM = $(addsuffix .sym,$(basename $(SRC)))
DISASM = $(addsuffix .disasm,$(basename $(SRC)))

CC = arm-unknown-bolthur-eabi-gcc
OBJDUMP = arm-unknown-bolthur-eabi-objdump
OBJCOPY = arm-unknown-bolthur-eabi-objcopy

ASFLAGS =
CFLAGS = -Wall -g -O0 -march=armv7-a -mtune=cortex-a7 -mfpu=neon-vfpv4 -mfloat-abi=hard
LDFLAGS = -static

TEST_CFLAGS = -march=armv7-a -mtune=cortex-a7 -mfpu=neon-vfpv4 -mfloat-abi=hard  -Wall -Wextra -Werror -Wpedantic -Wconversion -Wpacked -Wpacked-bitfield-compat -Wpacked-not-aligned -Wno-sign-conversion -Wno-unused-variable -ffreestanding -fno-exceptions -nodefaultlibs -std=c18 -fomit-frame-pointer -g -O0 -nostdlib
TEST_LDFLAGS = -T link.ld -static

all: $(BIN) $(DISASM) $(SYM)

test_generic.bin: test_generic.c
	$(CC) $(TEST_CFLAGS) $(TEST_LDFLAGS) $^ -o $@

test_mmap.bin: test_mmap.c
	$(CC) $(TEST_CFLAGS) $(TEST_LDFLAGS) $^ -o $@

test_process_exit.bin: test_process_exit.c
	$(CC) $(TEST_CFLAGS) $(TEST_LDFLAGS) $^ -o $@

test_thread_exit.bin: test_thread_exit.c
	$(CC) $(TEST_CFLAGS) $(TEST_LDFLAGS) $^ -o $@

%.bin: %.c
	$(CC) $(CFLAGS) $(LDFLAGS) $^ -o $@

%.disasm: %.bin
	$(OBJDUMP) -dx $^ > $@

%.sym: %.bin
	$(OBJCOPY) --only-keep-debug $^ $@

clean:
	rm $(BIN)
	rm $(DISASM)
	rm $(SYM)

.PHONY: all clean
