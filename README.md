# r3s0s

Some introduction, help and decisions about mist system kernel project.

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
```

### Building

```bash
# just call make for building the project
make clean && make
```

### Emulation

Emulation of the kernel project with qemu during development. When kernel debugging is necessary, add the parameter shorthand `-s` or the long version `-gdb tcp:1234` to qemu call.

```bash
# raspberry pi 2.1 kernel emulation
qemu-system-arm -m 1024 -M raspi2 -cpu cortex-a7 -no-reboot -serial stdio -kernel ./src/kernel/vendor/rpi/kernel.zwerg

# raspberry pi 2.2 kernel emulation
qemu-system-aarch64 -m 1024 -M raspi2 -cpu cortex-a53 -no-reboot -serial stdio -kernel ./src/kernel/vendor/rpi/kernel.zwerg

# raspberry pi 3 kernel emulation
qemu-system-aarch64 -m 1024 -M raspi2 -cpu cortex-a53 -no-reboot -serial stdio -kernel ./src/kernel/vendor/rpi/kernel.zwerg
```

## Planned support

Currently the following targets are planned to be supported:

* armv6 ( 32 bit arm rpi 1 and rpi zero )
* armv7 ( 32 bit arm rpi 2 )
* armv8 ( 64 bit arm rpi 2 and rpi 3 )

## Hardware for testing

* Raspberry PI 2 Model B ( rev 1.1 )
* Raspberry PI 3 Model B
