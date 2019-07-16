
# Things to be done

* [ ] Revise irq and isrs register handling
  * [ ] Add generic event system
  * [ ] Throw global events if an interrupt occurrs
  * [ ] Replace irq register handler by register event
* [ ] Replace magic values at serial init by defines
* [ ] FPU
  * [ ] Add push and pop of fpu registers within ivt stubs
  * [ ] Extend cpu structure if fpu is enabled
  * [ ] Extend undefined exception to check for fpu error with clear of flag
* [ ] Memory management
  * [ ] Add initial setup of paging with LPAE if enabled
  * [ ] Check for further splitting of entry point ( separation between 32bit and 64bit ) is necessary
  * [ ] Add lpae paging to kernel after boot setup
  * [ ] Consider and enable CPU related caches for performance
  * [ ] Check and use recursive page mapping
* [ ] Platform init
  * [ ] Add check for initrd loaded after kernel and move placement address for placement allocator beyond initrd
* [ ] Add gdb stub for debugging on remote device via serial port
  * [ ] Add remote debugging integration
  * [ ] Finish debug launch.json when remote debugging is possible
* [ ] Add multitasking
* [ ] Implement syscall handling via `swi`
* [ ] Add multithreading
* [ ] Add SMP support
  * [ ] Memory management
    * [ ] Prepare virtual memory management per core if smp is active
    * [ ] Specify cores per platform via autotools
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

* [ ] Create a draft for "build" system to create ready to boot images with platform driver/app packaging
  * [ ] Per platform initial ramdisk creation
* [ ] Create repository for building ported applications and libraries
  * [ ] Add newlib with patch for compilation
  * [ ] Add glibc with patch for compilation
