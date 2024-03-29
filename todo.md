
# Things to be done

* [ ] Change include from `<` and `>` for local files to `"`

* [ ] Replace thirdparty/dtc/libfdt by usage of the one from sysroot
* [ ] Replace magic values at serial init by defines
* [ ] FPU
  * [ ] Extend undefined exception to check for fpu error with clear of flag
* [ ] Add SMP support [see also](http://infocenter.arm.com/help/index.jsp?topic=/com.arm.doc.dai0425/ch04s07s01.html)
  * [ ] Memory management
    * [ ] Prepare virtual memory management per core if smp is active
  * [ ] Determine current running core within exceptions
  * [ ] Extend irq check to check corresponding cpu interrupt registers
* [ ] Documentation ( man pages or markdown )
  * [ ] Getting started after checkout
  * [ ] Cross-compiler toolchain
  * [ ] Configuring target overview

# Different projects, not kernel related things to be done

* [ ] Create a draft for "build" system to create ready to boot images with platform driver/app packaging
  * [ ] Per platform initial ramdisk creation
* [ ] Create repository for building ported applications and libraries
  * [ ] Add newlib with patch for compilation
  * [ ] Add glibc with patch for compilation
