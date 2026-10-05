
AM_CFLAGS = -DPROGRAM_NAME=\"usb/device/hid/hid\"

bin_PROGRAMS = hid
hid_DEPENDENCIES = \
  ${abs_top_builddir}/../library/usb/libusb.la \
  ${abs_top_builddir}/../library/collection/avl/libavl.la \
  ${abs_top_builddir}/../library/vfs/libvfs.la
hid_LDADD = \
  ${abs_top_builddir}/../library/usb/libusb.la \
  ${abs_top_builddir}/../library/collection/avl/libavl.la \
  ${abs_top_builddir}/../library/vfs/libvfs.la
hid_SOURCES = \
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
  rpc/get/application.c \
  rpc/get/driver.c \
  rpc/get/report.c \
  rpc/get/report_count.c \
  rpc/handler/register.c \
  rpc/handler/unregister.c \
  rpc/hid/attach.c \
  rpc/hid/detach.c \
  rpc/set/idle.c \
  rpc/set/report.c \
  rpc/init.c \
  handler.c \
  hid.c \
  main.c
hid_LDFLAGS = -all-static --static
