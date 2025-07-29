
AM_CFLAGS = -DPROGRAM_NAME=\"usb/device/hid/hid\"

bin_PROGRAMS = hid
hid_DEPENDENCIES = \
  ${abs_top_builddir}/../library/usb/libusb.la \
  ${abs_top_builddir}/../library/collection/avl/libavl.la
hid_LDADD = \
  ${abs_top_builddir}/../library/usb/libusb.la \
  ${abs_top_builddir}/../library/collection/avl/libavl.la
hid_SOURCES = \
  rpc/handler/register.c \
  rpc/handler/unregister.c \
  rpc/hid/attach.c \
  rpc/hid/deallocate.c \
  rpc/hid/detach.c \
  rpc/init.c \
  handler.c \
  main.c
hid_LDFLAGS = -all-static --static
