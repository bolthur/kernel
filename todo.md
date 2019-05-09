
# Things to be done

* [ ] Add irq and isrs register handling
  * [ ] Check and revise if necessary
* [ ] FPU
  * [ ] Add push and pop of fpu registers within ivt stubs
  * [ ] Extend cpu structure if fpu is enabled
* [ ] Memory management
  * [ ] Add normal setup of mmu during startup of kernel splitted to folders listed below
    * [ ] `kernel/arch/{architecture}/mm`
    * [ ] `kernel/arch/{architecture}/{sub architecture}/mm`
    * [ ] `vendor/{vendor}/mm`
  * [ ] Check for address extension within mmu is available, use it
  * [ ] Check for further splitting of entry point ( separation between 32bit and 64bit ) is necessary
  * [ ] Consider peripherals per vendor within mmu as not cachable
  * [ ] Consider and enable CPU related caches for performance
  * [ ] Consider KPTI ( kernel page table isolation ) for virtual memory management
  * [ ] Check and use recursive page mapping
* [ ] Platform init
  * [ ] Add check for initrd loaded after kernel and move placement address for placement allocator beyond initrd
* [ ] Ensure that kernel works still on real hardware also
* [ ] Memory management
  * [ ] Heap management for dynamic memory allocation done within `kernel` using architecture related code with avl tree
* [ ] Provide kernel implementation for `malloc`, `calloc`, `realloc` and `free` within `libk`
* [ ] AVL tree
  * [ ] Add generic avl tree library
* [ ] Add gdb stub for debugging on remote device via serial port
  * [ ] Add remote debugging integration
  * [ ] Finish debug launch.json when remote debugging is possible
* [ ] Add multitasking
* [ ] Implement syscall handling via `swi`
* [ ] FPU
  * [ ] Extend push of registers during exception to consider also fpu registers
  * [ ] Extend undefined exception to check for fpu error with clear of flag
* [ ] Add multithreading
* [ ] Add SMP support
  * [ ] Memory management
    * [ ] Prepare virtual memory management per core if smp is active
    * [ ] Specify cores per vendor via autotools
  * [ ] Determine current running core within exceptions
  * [ ] Extend irq check to check corresponding cpu interrupt registers
  * [ ] ...
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
* [ ] Support further platforms
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
* [ ] Move library `lib/tar` into separate repository and link them to kernel via git submodule
* [ ] Create repository for building ported applications and libraries
  * [ ] Add newlib with patch for compilation
  * [ ] Add glibc with patch for compilation
