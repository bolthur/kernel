# r3s0s

Some introduction, help and decisions about mist system kernel project.

## Building the project

### Configuring

```bash
mkdir build
cd build
# configure for i686 pc target
../configure --host i686-pc-elf --enable-device=pc
```

```bash
mkdir build
cd build
# configure for x86_64 pc target
../configure --host x86_64 --enable-device=pc
```

```bash
mkdir build
cd build
# configure for arm raspberry pi targets
../configure --host arm-none-eabi --enable-device=rpi2_1
#../configure --host arm-none-eabi --enable-device=rpi2_2
#../configure --host arm-none-eabi --enable-device=rpi3
```

### Building

```bash
# just call make for building the project
make clean && make
```

### Emulation

Emulation of the kernel project with qemu during development. When kernel debugging is necessary, add the parameter shorthand `-s` or the long version `-gdb tcp:1234` to qemu call.

```bash
# i386
qemu-system-i386 -cdrom image/pc/bootable.iso -m 256 -no-reboot

# x86_64
qemu-system-x86_64 -cdrom image/pc/bootable.iso -m 256 -no-reboot

# raspberry pi 2.1 kernel emulation
qemu-system-arm -m 1024 -M raspi2 -cpu cortex-a7 -no-reboot -serial stdio -kernel kernel.zwerg

# raspberry pi 2.2 kernel emulation
qemu-system-aarch64 -m 1024 -M raspi2 -cpu cortex-a53 -no-reboot -serial stdio -kernel kernel.zwerg

# raspberry pi 3 kernel emulation
qemu-system-aarch64 -m 1024 -M raspi2 -cpu cortex-a53 -no-reboot -serial stdio -kernel kernel.zwerg
```

## Planned support

Currently the following targets are planned to be supported:

* armv7 ( 32 bit arm rpi 2 )
* armv8 ( 32 & 64 bit arm rpi 2, rpi 3 )
* x86 ( 32 bit pc )
* x86_64 ( 64 bit pc )

## Hardware for testing

* Raspberry PI 2 Model B ( rev 1.1 )
* ~~Raspberry PI 2 Model B ( rev 1.2 )~~
* Raspberry PI 3 Model B

## May be added later

Following targets may be added after planned ones are on a good way and hardware for testing is existing:

* armv6
  * rpi 1 model a
  * rpi 1 model a+
  * rpi 1 model b
  * rpi 1 model b+
  * rpi zero
  * rpi zero w

## Other

```bash
# Compilation for rpi 1 ( armv6 if it will be done )
arm-none-eabi-gcc -O2 -mfpu=vfp -mfloat-abi=hard -march=armv6zk -mtune=arm1176jzf-s arm-test.c
```
