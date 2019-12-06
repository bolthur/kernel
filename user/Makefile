
SRCS = $(shell find -name '*.c')
OBJS = $(addsuffix .o,$(basename $(SRCS)))

CC = $(HOME)/.bolthur/cross/bin/arm-none-eabi-gcc
LD = $(HOME)/.bolthur/cross/bin/arm-none-eabi-ld

ASFLAGS =
CFLAGS = -march=armv7-a -mtune=cortex-a7 -mfpu=neon-vfpv4 -mfloat-abi=hard  -Wall -Wextra -Werror -Wpedantic -Wconversion -Wpacked -Wpacked-bitfield-compat -Wpacked-not-aligned -ffreestanding -fno-exceptions -nodefaultlibs -std=c18 -fomit-frame-pointer -g -O2 -nostdlib
LDFLAGS = -T link.ld

test.bin: $(OBJS)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $^

clean:
	rm $(OBJS)
	rm test.bin

.PHONY: clean
