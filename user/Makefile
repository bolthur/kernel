
SRC = $(shell find -name '*.c')
BIN = $(addsuffix .bin,$(basename $(SRC)))

CC = arm-bolthur-eabi-gcc

ASFLAGS =
CFLAGS = -Wall -g -march=armv7-a -mtune=cortex-a7 -mfpu=neon-vfpv4 -mfloat-abi=hard
LDFLAGS =

all: $(BIN)

%.bin: $(SRC)
	$(CC) $(CFLAGS) $(LDFLAGS) $^ -o $@ -specs=bolthur.specs

clean:
	rm $(BIN)

.PHONY: all clean
