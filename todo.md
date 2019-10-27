
# Things to be done

* [ ] Add gdb stub for debugging on remote device via serial port
  * [ ] Add remote debugging integration
  * [ ] Finish debug launch.json when remote debugging is possible
* [ ] Replace magic values at serial init by defines
* [ ] FPU
  * [ ] Add push and pop of fpu registers within ivt stubs
  * [ ] Extend cpu structure if fpu is enabled
  * [ ] Extend undefined exception to check for fpu error with clear of flag
* [ ] Memory management
  * [ ] Consider and enable CPU related caches for performance
* [ ] Multitasking
  * [ ] Add kernel side multitasking
  * [ ] Add user processes to multitasking
  * [ ] Add parsing of ELF files
  * [ ] Extend dummy initrd by test programs and execute them
* [ ] Implement syscall handling via `swi`
* [ ] TAR / initrd
  * [ ] Add initial ramdisk during boot which should be a simple tar file
  * [ ] Add parsing of initial ramdisk containing drivers or programs for startup
  * [ ] Check for drivers within folder "/driver"
* [ ] Multitasking
  * [ ] Add multithreading
* [ ] Add SMP support
  * [ ] Memory management
    * [ ] Prepare virtual memory management per core if smp is active
  * [ ] Determine current running core within exceptions
  * [ ] Extend irq check to check corresponding cpu interrupt registers
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
