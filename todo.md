
# Things to be done

* [ ] Replace thirdparty/dtc/libfdt by usage of the one from sysroot
* [ ] Replace magic values at serial init by defines
* [ ] FPU
  * [ ] Add push and pop of fpu registers within ivt stubs
  * [ ] Extend cpu structure if fpu is enabled
  * [ ] Extend undefined exception to check for fpu error with clear of flag
* [ ] Memory management
  * [ ] Consider and enable CPU related caches for performance
* [ ] TAR / initrd
  * [ ] Add parsing of initial ramdisk containing drivers or programs for startup
  * [ ] Check init driver within folder "/boot"
* [ ] Add SMP support [see also](http://infocenter.arm.com/help/index.jsp?topic=/com.arm.doc.dai0425/ch04s07s01.html)
  * [ ] Memory management
    * [ ] Prepare virtual memory management per core if smp is active
  * [ ] Determine current running core within exceptions
  * [ ] Extend irq check to check corresponding cpu interrupt registers
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
