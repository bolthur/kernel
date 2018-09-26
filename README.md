# MIST

_MIST_ is a recursive acronym for "MIST is somehow terrible". Below are some introductions, help and decisions about mist project. Originally it was derived from the german word mist, which might be translated as _crap_.

## Building the project

### Configuring

```bash
mkdir build
cd build
# configure for arm raspberry pi targets
#../configure --host arm-none-eabi --enable-device=rpi1_a
#../configure --host arm-none-eabi --enable-device=rpi1_a_plus
#../configure --host arm-none-eabi --enable-device=rpi1_b
#../configure --host arm-none-eabi --enable-device=rpi1_b_plus
../configure --host arm-none-eabi --enable-device=rpi2_b
#../configure --host aarch64-none-elf --enable-device=rpi2_b_rev2
#../configure --host aarch64-none-elf --enable-device=rpi3_b
#../configure --host aarch64-none-elf --enable-device=rpi3_b_plus
#../configure --host arm-none-eabi --enable-device=rpi_zero
#../configure --host arm-none-eabi --enable-device=rpi_zero_w
#../configure --host arm-none-eabi --enable-device=beagleboard
#../configure --host arm-none-eabi --enable-device=n8x0
#../configure --host arm-none-eabi --enable-device=n900
```

### Building

```bash
# just call make for building the project
make clean && make
```

### Real hardware

Getting some output on real hardware, like debug output is done via serial. In case of raspberry pi it is UART0. So to get execution output, you'll need an TTL-RS232 to USB convert cable. When that is existing, wire it up like on the following [page](https://blog.christophersmart.com/2016/10/27/building-and-booting-upstream-linux-and-u-boot-for-raspberry-pi-23-arm-boards/) and connect it to the development machine. The most simple way to get the serial output is using screen with the connected usb to serial.

```bash
# Connect to usb tty port via screen
screen /dev/ttyUSB0 115200

# Exit tty screen session
Ctrl+a Shift+A y
```

### Emulation

Emulation of the kernel project with qemu during development. When kernel debugging is necessary, add the parameter shorthand `-s` or the long version `-gdb tcp:1234` to qemu call.

```bash
# raspberry pi 1 kernel emulation
qemu-system-arm -m 256 -M versatilepb -cpu arm1176 -no-reboot -serial stdio -kernel ./src/kernel/vendor/rpi/kernel.zwerg

# raspberry pi 2.1 kernel emulation
qemu-system-arm -m 1024 -M raspi2 -cpu cortex-a7 -no-reboot -serial stdio -kernel ./src/kernel/vendor/rpi/kernel.zwerg

# raspberry pi 2.2 kernel emulation
qemu-system-aarch64 -m 1024 -M raspi2 -cpu cortex-a53 -no-reboot -serial stdio -kernel ./src/kernel/vendor/rpi/kernel.zwerg

# raspberry pi 3 kernel emulation
qemu-system-aarch64 -m 1024 -M raspi2 -cpu cortex-a53 -no-reboot -serial stdio -kernel ./src/kernel/vendor/rpi/kernel.zwerg
```

## Planned support

Currently the following targets are planned to be supported:

### Broadcom SoC

* armv6 ( 32 bit )
  * RPI A
  * RPI A+
  * RPI B
  * RPI B+
  * RPI zero
  * RPI zero w

* armv7-a ( 32 bit )
  * RPI 2B r.1

* armv8-a ( 64 bit - rpi 2 r.2 and rpi 3 series )
  * RPI 2B r.2
  * RPI 3B
  * RPI 3B+

### Texas Instruments OMAP SoC

* armv6 ( 32 bit - OMAP 2420 )
  * nokia n800
  * nokia n810

* armv7-a ( 32 bit - OMAP3530 )
  * beagle board
  * nokia n900

## List of real hardware for testing

* Raspberry PI 2 Model B ( rev 1.1 )
* Raspberry PI 3 Model B
