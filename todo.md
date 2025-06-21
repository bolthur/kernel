
# Things to be done

## Kernel

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
* [ ] Adjust message passing for rpc calls
  * [ ] Allocate page size for mailbox ( linked message list )
  * [ ] Map mailbox temporarily
  * [ ] Check for enough space is left
  * [ ] Add new rpc parameter to mailbox
  * [ ] Try to switch to process handling rpc

## Different projects, not kernel related things to be done

* [ ] Create a draft for "build" system to create ready to boot images with platform driver/app packaging
  * [ ] Per platform initial ramdisk creation

## VFS

* [ ] Revise vfs handles to tree built by complete path
* [x] Replace vfs tree by simple mount point list
* [x] VFS shall have 2 different mount points before mount is called
  * [x] /dev
  * [x] /ramdisk
* [x] VFS shall have 2 additional mount points when mounting root and boot was successful
  * [x] /
  * [x] /boot
* [x] Filesystem access
  * [x] Mounting of / and /boot shall be done by a chains like the following:
    * [x] process->vfs->fs/fs->fs/ext->storage/sd->fs/ext->fs/fs->vfs->process
    * [x] process->vfs->fs/fs->fs/fat->storage/sd->fs/fat->fs/fs->vfs->process

## Servers

* [ ] Adjust rpc structure to not peek messages from kernel but fetch from mailbox
