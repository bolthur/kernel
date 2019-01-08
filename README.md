# bolthur kernel

bolthur kernel project. Below are some introductions, help and decisions about bolthur kernel project.

## Things to be done

* [x] Cleanup current code mess
  * [x] Move memory barrier header to arm as it is arm related
  * [x] Change peripheral base from constant to function due to later virtual remap
  * [x] Strip out ATAG and flat device tree parsing into library
  * [x] Prefix folders used by automake with an underscore
* [x] Serial output done within `kernel/vendor/{vendor}`
* [x] TTY for printing debug messages done within `kernel/vendor/{vendor}`
  * [x] printf implementation for kernel environment
* [x] Interrupt requests and fast interrupts
* [x] Add memory barriers for arm necessary for e.g. mailbox on rpi
* [ ] Reorder code
  * [x] Move source files out of folder `src`
  * [ ] Restructore code
* [ ] Memory management
  * [ ] Physical memory management
    * [ ] Get max memory from vendor
      * [ ] Gather rpi memory from mailbox ( store physical memory map generally per vendor )
      * [ ] Initialize memory bitmap within vendor
    * [ ] Generic physical handling done within `kernel` via memory bitmap
  * [ ] Virtual memory management done within `kernel/arch/{architecture}/{sub architecture}`
  * [ ] Add relocation of kernel to higher half after mmu setup
    * [ ] kernel load address
      * [ ] `0xC0008000` for 32bit
      * [ ] `0xffffff0000080000` for 64bit
  * [ ] Heap management for dynamic memory allocation done within `kernel` using architecture related code
  * [ ] Consider peripherals per board within mmu as not cachable
  * [ ] Enable CPU caches
* [ ] Event system for mapping service routines `Needs to be planned`
  * [ ] ~~Generic code for `register` and `unregister` an event done within `kernel`~~
  * [ ] ~~Vendor related mapping~~
    * [ ] ~~Interrupt requests~~
    * [ ] ~~Fast interrupt requests~~
    * [ ] ~~Software interrupts~~
* [ ] Provide kernel implementation for `malloc`, `calloc`, `realloc` and `free`
* [ ] Add gdb stub for debugging on remote device via serial port
  * [ ] Use dynamic memory allocation
  * [ ] Find better place for `serial_init` than `tty_init`
  * [ ] Finish debug launch.json when remote debugging is possible

## Unordered list of things to be done and ideas

* [ ] Evaluate switch from custom cross compiler toolchain to llvm
  * [ ] Check remote debugging capabilities
* [ ] AVL tree
  * [ ] Add generic avl tree library
  * [ ] Implement heap management using avl tree library
* [ ] TAR
  * [ ] Add generic tar library for reading tar files
  * [ ] Add initial ramdisk during boot which should be a simple tar file
  * [ ] Add parsing of initial ramdisk containing drivers or programs for startup
* [ ] Device tree
  * [ ] Add device tree library
  * [ ] Extend automake by option for use device tree
  * [ ] Add parse of device tree when compiled in via option
  * [ ] Debug output, when no device tree has been found
* [ ] ATAG
  * [ ] Add own atag library
  * [ ] Extend automake by option for use atag
  * [ ] Add parse of atag when compiled in via option
  * [ ] Debug output, when no atag has been passed
* [ ] Add irq and isrs register handling
  * [x] Get irq with cpu mode switch and register dump working
  * [x] Merge irq functions with isrs functions where possible
  * [ ] Prohibit mapping of interrupt routines
  * [ ] Add event system with `register` and `unregister`
    * [ ] Provide map of events for `irq`, `fiq`, `swi`
    * [ ] Fire events for `irq`, `fiq`, `swi`
* [ ] Add virtual file system for initrd
* [ ] Create a draft for build system with vendor driver/app packaging
  * [ ] Per vendor initial ramdisk creation
* [ ] Documentation ( man pages or markdown )
  * [ ] Getting started after checkout
  * [ ] Cross compiler toolchain
  * [ ] Configuring target overview

## Building the project

### Autotools related

Initial necessary commands after checkout:

```bash
aclocal -I m4
autoheader
automake --foreign --add-missing
autoconf
```

Necessary commands after adding new files:

```bash
autoreconf -i
```

### Configuring

```bash
mkdir build
cd build
### configure with one of the following commands
../configure --host arm-none-eabi --enable-device=rpi1_a --enable-debug --enable-kernel-print
../configure --host arm-none-eabi --enable-device=rpi1_a_plus --enable-debug --enable-kernel-print
../configure --host arm-none-eabi --enable-device=rpi1_b --enable-debug --enable-kernel-print
../configure --host arm-none-eabi --enable-device=rpi1_b_plus --enable-debug --enable-kernel-print
../configure --host arm-none-eabi --enable-device=rpi2_b --enable-debug --enable-kernel-print
../configure --host aarch64-none-elf --enable-device=rpi2_b_rev2 --enable-debug --enable-kernel-print
../configure --host aarch64-none-elf --enable-device=rpi3_b --enable-debug --enable-kernel-print
../configure --host aarch64-none-elf --enable-device=rpi3_b_plus --enable-debug --enable-kernel-print
../configure --host arm-none-eabi --enable-device=rpi_zero --enable-debug --enable-kernel-print
../configure --host arm-none-eabi --enable-device=rpi_zero_w --enable-debug --enable-kernel-print
```

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

Emulation of the project with qemu during development is **not** recommended. The platforms itself are most time very limited or completely not supported. Raspberry pi is very limited and lack necessary , also the n900 or the beagleboard are completely not supported. We recommend testing on real hardware with remote debugging via serial port.

Emulation of the kernel project with qemu during development may be done at all with the following commands. When kernel debugging is necessary, add the parameter shorthand `-s` or the long version `-gdb tcp:1234` to qemu call:

```bash
# raspberry pi 2B rev 1 kernel emulation
qemu-system-arm -M raspi2 -cpu cortex-a7 -m 1G -no-reboot -serial stdio -kernel ./kernel/vendor/rpi/kernel_qemu -dtb ../kernel/vendor/rpi/device/bcm2709-rpi-2-b.dtb

# raspberry pi 2B rev 2 kernel emulation ( to be tested )
qemu-system-aarch64 -m 1024 -M raspi2 -cpu cortex-a53 -m 1G -no-reboot -serial stdio -kernel ./kernel/vendor/rpi/kernel_qemu -dtb ../kernel/vendor/rpi/device/bcm2709-rpi-2-b.dtb

# raspberry pi 3B and 3B+ kernel emulation
qemu-system-aarch64 -m 1024 -M raspi3 -cpu cortex-a53 -m 1G -no-reboot -serial stdio -kernel ./kernel/vendor/rpi/kernel_qemu -dtb ../kernel/vendor/rpi/device/bcm2710-rpi-3-b.dtb
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

## List of real hardware for testing

* Raspberry PI 2 Model B ( rev 1.1 )
* Raspberry PI 3 Model B
