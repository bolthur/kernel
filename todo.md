
# Things to be done

* [ ] Replace magic values at serial init by defines
* [ ] FPU
  * [ ] Extend undefined exception to check for fpu error with clear of flag
* [ ] Add SMP support [see also](http://infocenter.arm.com/help/index.jsp?topic=/com.arm.doc.dai0425/ch04s07s01.html)
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

# VFS

* [ ] Revise vfs handles to tree built by complete path
* [x] Replace vfs tree by simple mount point list
* [ ] VFS shall have 3 different mount points before mount is called
  * [ ] /dev
  * [ ] /manager
  * [ ] /ramdisk
* [ ] VFS shall have 2 additional mount points when mounting root and boot was successful
  * [ ] /
  * [ ] /boot
* [ ] Filesystem access
  * [ ] Revise ramdisk
    * [ ] Launch fs/fs after /dev was started
    * [ ] fs/fs shall watch folder /dev/storage for changes
    * [ ] Launch fs/ramdisk after fs/fs was started with registration for mount type "ramdisk"
    * [ ] Boot registers ramdisk at /dev/storage/ramdisk
    * [ ] fs manager queries "partitions" (there will be only one) of /dev/storage/ramdisk and adds them as /dev/storage/ramdiskX
    * [ ] init stage 1 mounts /dev/storage/ramdisk1 to /ramdisk
      * [ ] vfs routes the mount request, since it's not existing yet to fs/fs
      * [ ] fs/fs saves mount point /ramdisk with handler type ( should be registered fs/ramdisk )
    * [ ] file/folder access chain (boot is managing the direct ramdisk access):
      * [ ] process->vfs->fs/fs->fs/ramdisk->boot->fs/ramdisk->fs/fs->vfs->process
  * [ ] Mounting of / and /boot shall be done by a chains like the following:
    * [ ] process->vfs->fs/fs->fs/ext->storage/sd->fs/ext->fs/fs->vfs->process
    * [ ] process->vfs->fs/fs->fs/fat->storage/sd->fs/fat->fs/fs->vfs->process
