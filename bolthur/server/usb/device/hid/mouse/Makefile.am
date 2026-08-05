
AM_CFLAGS = -DPROGRAM_NAME=\"usb/device/hid/mouse\"

bin_PROGRAMS = mouse
mouse_DEPENDENCIES = \
  ${abs_top_builddir}/../library/hid/libhid.la \
  ${abs_top_builddir}/../library/usb/libusb.la \
  ${abs_top_builddir}/../library/vfs/libvfs.la \
  ${abs_top_builddir}/../library/util/libutil.la \
  ${abs_top_builddir}/../library/collection/list/liblist.la
mouse_LDADD = \
  ${abs_top_builddir}/../library/hid/libhid.la \
  ${abs_top_builddir}/../library/usb/libusb.la \
  ${abs_top_builddir}/../library/vfs/libvfs.la \
  ${abs_top_builddir}/../library/util/libutil.la \
  ${abs_top_builddir}/../library/collection/list/liblist.la
mouse_SOURCES = \
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
  rpc/handler/register.c \
  rpc/handler/unregister.c \
  rpc/mouse/attach.c \
  rpc/mouse/detach.c \
  rpc/mouse/mouse.c \
  rpc/init.c \
  handler.c \
  main.c \
  mouse.c
mouse_LDFLAGS = -all-static --static
