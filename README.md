# bolthur kernel [![License: GPL v3](https://img.shields.io/badge/License-GPLv3-blue.svg)](./LICENSE) [![Code of Conduct](https://img.shields.io/badge/%E2%9D%A4-code%20of%20conduct-blue.svg?style=flat)](./.github/CODE_OF_CONDUCT.md) [![Contribution Guide](https://img.shields.io/badge/contributions-welcome-brightgreen.svg?style=flat)](./.github/CONTRIBUTING.md) ![version](https://img.shields.io/badge/version-none-blue.svg?maxAge=2592000)

bolthur kernel project.

_Copyright (C) 2017 - 2019 bolthur project_

## Supported platforms

There are no completely supported platforms yet.

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

## List of real hardware for testing

* Raspberry PI 2 Model B ( rev 1.1 )
* Raspberry PI 3 Model B
* Raspberry PI Zero W

## Building the project

### Autotools related

Initial necessary commands after checkout:

```bash
./autogen.sh
```

Necessary commands after adding new files to autotools executed within project root:

```bash
autoreconf -i
```

### Configuring

```bash
mkdir build
cd build
### configure with one of the following commands
../configure --host arm-none-eabi --enable-device=rpi2_b_rev1 --enable-debug --enable-output
../configure --host aarch64-none-elf --enable-device=rpi3_b --enable-debug --enable-output
../configure --host arm-none-eabi --enable-device=rpi_zero_w --enable-debug --enable-output
```

Possible additional parameters to `--host` and `--enable-device`:

* `--enable-debug` enables remote debugging mode
* `--enable-output` enables kernel output
* `--enable-serial-tty` switch from framebuffer output to serial output
* `--enable-output-mm-phys` activate tty output of physical memory manager ( slows down kernel totally )
* `--enable-output-mm-virt` activate tty output of virtual memory manager
* `--enable-output-mm-heap` activate tty output of kernel heap
* `--enable-output-mailbox` activate tty output of mailbox implementation

### Building

```bash
# just call make for building the project
make clean && make
```

### Remote debugging

For remote debugging configure the kernel with `--enable-debug`, rebuild and copy it to remote device. After that, depending on the remote arch, execute one of the following commands

```bash
### debug 32 bit arm device
../scripts/opt/cross/bin/arm-none-eabi-gdb -b 115200 --tty=/dev/ttyUSB0 ./kernel/vendor/rpi/kernel.zwerg ./kernel/vendor/rpi/kernel.map

### debug 64 bit arm device
../scripts/opt/cross/bin/aarch64-none-elf-gdb -b 115200 --tty=/dev/ttyUSB0 ./kernel/vendor/rpi/kernel.zwerg ./kernel/vendor/rpi/kernel.map
```

### Real hardware

Getting some debug-print on real hardware, like debug debug-print is done via serial. In case of raspberry pi it is UART0. So to get execution debug-print, you'll need an TTL-RS232 to USB convert cable. When that is existing, wire it up like on the following [page](https://blog.christophersmart.com/2016/10/27/building-and-booting-upstream-linux-and-u-boot-for-raspberry-pi-23-arm-boards/) and connect it to the development machine. The most simple way to get the serial debug-print is using screen with the connected usb to serial.

```bash
# Connect to usb tty port via screen
screen /dev/ttyUSB0 115200

# Exit tty screen session
Ctrl+a Shift+k y
```

### Emulation

Emulation of the project with qemu during development is **not** recommended. The platforms itself are most time very limited or completely not supported. Raspberry pi is very limited and lack necessary features, also the n900 or the beagleboard are completely not supported. We recommend testing on real hardware with remote debugging via serial port.

Emulation of the kernel project with qemu during development may be done at all with the following commands. When kernel debugging is necessary, add the parameter shorthand `-s` or the long version `-gdb tcp:1234` to qemu call:

```bash
# raspberry pi 2B rev 1 kernel emulation
qemu-system-arm -M raspi2 -cpu cortex-a7 -m 1G -no-reboot -serial stdio -kernel ./kernel/vendor/rpi/kernel.zwerg

# raspberry pi 3B kernel emulation
qemu-system-aarch64 -m 1024 -M raspi3 -cpu cortex-a53 -m 1G -no-reboot -serial stdio -kernel ./kernel/vendor/rpi/kernel.zwerg
```
