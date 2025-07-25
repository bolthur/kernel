
AM_CFLAGS = -DPROGRAM_NAME=\"usb/device/hub\"

bin_PROGRAMS = hub
hub_DEPENDENCIES = \
  ${abs_top_builddir}/../library/usb/libusb.la
hub_LDADD = \
  ${abs_top_builddir}/../library/usb/libusb.la
hub_SOURCES = \
  rpc/hub/attach.c \
  rpc/init.c \
  main.c
hub_LDFLAGS = -all-static --static
