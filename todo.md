
# Things to be done

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
* [x] Restructure
  * [x] Move source files out of folder `src`
  * [x] Restructore code
  * [x] Merge defines `KERNEL_DEBUG_PRINT` and `DEBUG`
  * [x] Reduce devices within `m4/host.m4` and `configure.ac` to existing hardware
  * [x] Provide `autogen.sh` for simple startup after checkout / download
* [x] Memory management
  * [x] Physical memory management
    * [x] Get max memory from vendor ( store physical memory map generally per vendor )
    * [x] Setup memory bitmap within vendor
    * [x] Mark physical peripheral address areas per vendor as used
* [ ] Add irq and isrs register handling
  * [x] Get irq with cpu mode switch and register dump working
  * [x] Merge irq functions with isrs functions where possible
  * [ ] Check and revise if necessary
* [x] FPU
  * [x] Check and revise fpu code
  * [x] Check and remove direct floating point usage within kernel except saving of floating point registers
* [ ] Memory management
  * [x] Add higher half define via vendor autotools
  * [x] Move static naked aligned function `interrupt_vector_table` into assembler stub
  * [x] Move stack creation into own assembly stub
  * [ ] Add creation of initial mmu during startup of vendor
    * [ ] Create assembly stub for initial identity mapping
    * [ ] Extend assembly stub for initial identity mapping by higher half mapping if set
  * [ ] Ensure that kernel with no higher half define still works as expected
  * [ ] Check and revise stack setup for interrupt vector table regarding possible higher half
  * [ ] Add normal setup of mmu during startup of kernel splitted to folders listed below
    * [ ] `kernel/arch/{architecture}/mm`
    * [ ] `kernel/arch/{architecture}/{sub architecture}/mm`
    * [ ] `vendor/{vendor}/mm`
  * [ ] Check for address extension within mmu is available, use it
  * [ ] Check for further splitting of entry point ( separation between 32bit and 64bit ) is necessary
  * [ ] Consider peripherals per vendor within mmu as not cachable
  * [ ] Consider and enable CPU related caches for performance
* [ ] Ensure that kernel works still on real hardware also
* [ ] AVL tree
  * [ ] Add generic avl tree library
* [ ] Memory management
  * [ ] Heap management for dynamic memory allocation done within `kernel` using architecture related code with avl tree
* [ ] `libc` rework for further kernel development
  * [ ] Rename to `libk` and remove all non kernel related parts
  * [ ] Provide kernel implementation for `malloc`, `calloc`, `realloc` and `free`
* [ ] Add gdb stub for debugging on remote device via serial port
  * [ ] Rework existing unfinished remote debugging code
  * [ ] Find better place for `serial_init` than `tty_init`
  * [ ] Finalize remote debugging integration
  * [ ] Finish debug launch.json when remote debugging is possible
* [ ] Add multitasking
* [ ] Implement syscall handling via `swi`
* [ ] FPU
  * [ ] Enable fpu only per process when fpu exception has been thrown
  * [ ] Extend push of registers during exception to consider also fpu registers
  * [ ] Extend undefined exception to check for fpu error with clear of flag
* [ ] Add multithreading
* [ ] Add SMP support
  * [x] Add smp define per vendor via autotools
  * [x] Add smp core amount define per vendor via autotools
  * [ ] Memory management
    * [ ] Prepare virtual memory management per core if smp is active
    * [ ] Specify cores per vendor via autotools
  * [ ] Determine current running core within exceptions
  * [ ] Extend irq check to check corresponding cpu interrupt registers
  * [ ] ...
* [x] TTY changes
  * [x] Add switch to use serial tty for debug output
  * [x] Get text printing via framebuffer to work
  * [x] Use framebuffer as default tty
  * [x] Finalize console implementation for framebuffer
* [ ] TAR
  * [ ] Add generic tar library for reading tar files
  * [ ] Add initial ramdisk during boot which should be a simple tar file
  * [ ] Add parsing of initial ramdisk containing drivers or programs for startup
* [ ] Add virtual file system for initrd
  * [ ] RPI related
    * [ ] Determine one of the two options to choose, or support both ( via config.txt )
      * [ ] Handle initrd to be added after kernel
      * [ ] Handle initrd loaded to fixed address set per board
    * [ ] Extend memory management
      * [ ] Mark initrd within physical memory manager as used
      * [ ] Check and extend virtual memory management if necessary
  * [ ] Add generic vfs implementation using tar initrd
* [ ] Add parsing of ELF files
  * [ ] Check for executable elf programs within initrd and execute them
* [ ] Device tree
  * [ ] Add device tree library
  * [ ] Extend automake by option for use device tree
  * [ ] Add parse of device tree when compiled in via option
  * [ ] Debug output, when no device tree has been found and use kernel defaults
* [ ] ATAG
  * [ ] Add atag library
  * [ ] Extend automake by option for use atag
  * [ ] Add parse of atag when compiled in via option
  * [ ] Debug output, when no atag has been passed and use kernel defaults
* [ ] Autotools / Compilation
  * [ ] Check and revise kernel to work with all optimization levels
  * [x] Check autotools setup to get compilation with more than one core to work
* [ ] Support further platforms
  * [ ] Test and fix rpi 2 rev. 1 support with real hardware
  * [ ] Add rpi zero support ( armv6 32 bit only with one cpu )
  * [ ] Add rpi 3 support ( armv8 64 bit with smp )
  * [ ] Add rpi 3 support ( armv8 32 bit with smp )
* [ ] Documentation ( man pages or markdown )
  * [ ] Getting started after checkout
  * [ ] Cross compiler toolchain
  * [ ] Configuring target overview

# Different projects, not kernel related things to be done

* [ ] Create a draft for "build" system to create ready to boot images with vendor driver/app packaging
  * [ ] Per vendor initial ramdisk creation
* [ ] Move libraries `lib/tar`, `lib/avl`, `lib/atag` and `lib/fdt` into separate repositories and link them to kernel via git submodule if kernel related
* [ ] Create repository for building ported applications and libraries
  * [ ] Add newlib with patch for compilation
  * [ ] Add glibc with patch for compilation
