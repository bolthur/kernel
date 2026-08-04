
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
