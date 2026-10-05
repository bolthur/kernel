
bin_PROGRAMS = keyboard
keyboard_CFLAGS = \
  $(CONFINI_CFLAGS) \
  -DPROGRAM_NAME=\"usb/device/hid/keyboard\"
keyboard_DEPENDENCIES = \
  ${abs_top_builddir}/../library/hid/libhid.la \
  ${abs_top_builddir}/../library/usb/libusb.la \
  ${abs_top_builddir}/../library/util/libutil.la \
  ${abs_top_builddir}/../library/vfs/libvfs.la
keyboard_LDADD = \
  ${abs_top_builddir}/../library/hid/libhid.la \
  ${abs_top_builddir}/../library/usb/libusb.la \
  ${abs_top_builddir}/../library/util/libutil.la \
  $(CONFINI_LIBS) \
  ${abs_top_builddir}/../library/vfs/libvfs.la
keyboard_SOURCES = \
  rpc/generic/add.c \
  rpc/generic/close.c \
  rpc/generic/exec.c \
  rpc/generic/exit.c \
  rpc/generic/fork.c \
  rpc/generic/ioctl.c \
  rpc/generic/open.c \
  rpc/generic/read.c \
  rpc/generic/remove.c \
  rpc/generic/seek.c \
  rpc/generic/stat.c \
  rpc/generic/write.c \
  rpc/keyboard/attach.c \
  rpc/keyboard/detach.c \
  rpc/keyboard/key.c \
  rpc/init.c \
  keyboard.c \
  keymap.c \
  main.c
keyboard_LDFLAGS = -all-static --static
