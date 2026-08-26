
AM_CFLAGS = -DPROGRAM_NAME=\"usb/hcd\"

bin_PROGRAMS = hcd
hcd_DEPENDENCIES = \
  ${abs_top_builddir}/../library/util/libutil.la \
  ${abs_top_builddir}/../library/platform/raspi/iomem/libiomem.la \
  ${abs_top_builddir}/../library/vfs/libvfs.la \
  ${abs_top_builddir}/../library/usb/libusb.la
hcd_LDADD = \
  ${abs_top_builddir}/../library/util/libutil.la \
  ${abs_top_builddir}/../library/platform/raspi/iomem/libiomem.la \
  ${abs_top_builddir}/../library/vfs/libvfs.la \
  ${abs_top_builddir}/../library/usb/libusb.la
hcd_SOURCES = \
  rpc/default/timer.c \
  rpc/interrupt/handle.c \
  rpc/poll/interrupt.c \
  rpc/stop/transmission.c \
  rpc/submit/message.c \
  barrier.c \
  rpc/init.c \
  dwhci.c \
  dwhciroothub.c \
  main.c \
  mmio.c \
  response.c \
  timer.c
hcd_LDFLAGS = -all-static --static
