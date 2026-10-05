
AM_CFLAGS = -DPROGRAM_NAME=\"usb/device/hub\"

bin_PROGRAMS = hub
hub_DEPENDENCIES = \
  ${abs_top_builddir}/../library/usb/libusb.la \
  ${abs_top_builddir}/../library/vfs/libvfs.la
hub_LDADD = \
  ${abs_top_builddir}/../library/usb/libusb.la \
  ${abs_top_builddir}/../library/vfs/libvfs.la
hub_SOURCES = \
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
  rpc/hub/check/change.c \
  rpc/hub/child/detach.c \
  rpc/hub/attach.c \
  rpc/hub/detach.c \
  rpc/init.c \
  hub.c \
  main.c
hub_LDFLAGS = -all-static --static
