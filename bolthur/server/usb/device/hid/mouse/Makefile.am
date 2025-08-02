
AM_CFLAGS = -DPROGRAM_NAME=\"usb/device/hid/mouse\"

bin_PROGRAMS = mouse
mouse_DEPENDENCIES = \
  ${abs_top_builddir}/../library/hid/libhid.la \
  ${abs_top_builddir}/../library/usb/libusb.la
mouse_LDADD = \
  ${abs_top_builddir}/../library/hid/libhid.la \
  ${abs_top_builddir}/../library/usb/libusb.la
mouse_SOURCES = \
  rpc/mouse/attach.c \
  rpc/mouse/deallocate.c \
  rpc/mouse/detach.c \
  rpc/init.c \
  main.c \
  mouse.c
mouse_LDFLAGS = -all-static --static
