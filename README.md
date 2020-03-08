# kernel

bolthur/kernel project.
_Copyright (C) 2018 - 2019 bolthur project_

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

* armv8-a ( 64 bit - rpi 2 r.2, rpi 3 and rpi 4 series )
  * RPI 2B r.2
  * RPI 3B
  * RPI 3B+
  * RPI 4

## List of real hardware for testing

* Raspberry PI 2 Model B ( rev 1.1 )
* Raspberry PI 3 Model B
* Raspberry PI Zero W

## Building the project

### Autotools related

Initial necessary command after checkout or adding new files autotools executed within project root:

```bash
autoreconf -iv
```

### Configuring

```bash
mkdir build
cd build
### configure with one of the following commands
../configure --host arm-bolthur-eabi --enable-device=rpi2_b_rev1 --enable-debug --enable-output
../configure --host aarch64-bolthur-elf --enable-device=rpi3_b --enable-debug --enable-output
../configure --host arm-bolthur-eabi --enable-device=rpi_zero_w --enable-debug --enable-output
```

Possible additional parameters to `--host` and `--enable-device`:

* `--with-debug-symbols` enables remote debugging mode
* `--with-optimization-level=x` sets optimization from default ( 2 ) to specified one
* `--enable-output` enables kernel output
* `--enable-output-mm-phys` activate tty output of physical memory manager ( slows down kernel totally )
* `--enable-output-mm-virt` activate tty output of virtual memory manager
* `--enable-output-mm-heap` activate tty output of kernel heap
* `--enable-output-mm-placement` activate tty output of placement allocator
* `--enable-output-mailbox` activate tty output of mailbox implementation
* `--enable-output-timer` activate tty output of timer implementation
* `--enable-output-initrd` activate initrd implementation output
* `--enable-output-event` activate event implementation output
* `--enable-output-process` activate process implementation output
* `--enable-output-interrupt` activate interrupt request output
* `--enable-output-exception` activate exception handler output
* `--enable-output-elf` activate elf routine output
* `--enable-output-platform` activate platform initialization output
* `--enable-output-syscall` activate syscall output
* `--enable-output-serial` activate serial handling output
* `--enable-remote-debug` activate remote debugging

### Building

```bash
# just call make for building the project
make clean && make
```

### Remote debugging

For remote debugging configure the kernel with `--enable-debug`, rebuild and copy it to remote device. After that, depending on the remote arch, execute one of the following commands

```bash
### debug 32 bit arm device
../scripts/opt/cross/bin/arm-none-eabi-gdb -b 115200 --tty=/dev/ttyUSB0 ./platform/rpi/kernel.elf ./platform/rpi/kernel.map

### debug 64 bit arm device
../scripts/opt/cross/bin/aarch64-none-elf-gdb -b 115200 --tty=/dev/ttyUSB0 ./platform/rpi/kernel.elf ./platform/rpi/kernel.map
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

Emulation of the kernel project with qemu during development may be done at all with the following commands. When kernel debugging is necessary, add the parameter shorthand `-s -S` or the long version `-gdb tcp:1234` to qemu call:

```bash
# raspberry pi 2B rev 1 kernel emulation
qemu-system-arm -M raspi2 -cpu cortex-a7 -m 1G -no-reboot -serial stdio -kernel ./src/target/rpi/kernel.elf -s -S

# raspberry pi 2B rev 2 kernel emulation
qemu-system-arm -M raspi2 -cpu cortex-a7 -m 1G -no-reboot -serial stdio -kernel ./src/target/rpi/kernel.elf -s -S
qemu-system-aarch64 -M raspi2 -cpu cortex-a7 -m 1G -no-reboot -serial stdio -kernel ./src/target/rpi/kernel.elf -s -S

# raspberry pi 3B kernel emulation
qemu-system-arm -M raspi3 -cpu cortex-a53 -m 1G -no-reboot -serial stdio -kernel ./src/target/rpi/kernel.elf -s -S
qemu-system-aarch64 -M raspi3 -cpu cortex-a53 -m 1G -no-reboot -serial stdio -kernel ./src/target/rpi/kernel.elf -s -S
```

Starting the debugger from within build folder without any additional commands necessary would be done as follows:

```bash
/path/to/arm-none-eabi-gdb src/target/rpi/kernel7.sym
```

### Test remote serial debugging with qemu

Testing remote debugging with qemu within development from build directory.

```bash
/path/to/arm-none-eabi-gdb src/target/rpi/kernel7_qemu.sym
```

Within gdb execute following commands:

```
file src/target/rpi/kernel7_qemu.sym
set verbose on
set debug remote 1
set architecture armv7
set serial baud 115200
target remote /dev/pts/3
```

```
file src/target/rpi/kernel7_qemu.sym
set architecture armv7
set serial baud 115200
target remote /dev/pts/3
```
