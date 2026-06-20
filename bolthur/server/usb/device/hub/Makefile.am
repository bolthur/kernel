
AM_CFLAGS = -DPROGRAM_NAME=\"usb/device/hub\"

bin_PROGRAMS = hub
hub_DEPENDENCIES = \
  ${abs_top_builddir}/../library/usb/libusb.la \
  ${abs_top_builddir}/../library/vfs/libvfs.la
hub_LDADD = \
  ${abs_top_builddir}/../library/usb/libusb.la \
  ${abs_top_builddir}/../library/vfs/libvfs.la
hub_SOURCES = \
  rpc/hub/check/change.c \
  rpc/hub/check/connection.c \
  rpc/hub/child/detach.c \
  rpc/hub/child/reset.c \
  rpc/hub/attach.c \
  rpc/hub/detach.c \
  rpc/init.c \
  hub.c \
  main.c
hub_LDFLAGS = -all-static --static
