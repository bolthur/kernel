
bin_PROGRAMS = keyboard
keyboard_CFLAGS = \
  $(CONFINI_CFLAGS) \
  -DPROGRAM_NAME=\"usb/device/hid/keyboard\"
keyboard_DEPENDENCIES = \
  ${abs_top_builddir}/../library/hid/libhid.la \
  ${abs_top_builddir}/../library/usb/libusb.la \
  ${abs_top_builddir}/../library/util/libutil.la
keyboard_LDADD = \
  ${abs_top_builddir}/../library/hid/libhid.la \
  ${abs_top_builddir}/../library/usb/libusb.la \
  ${abs_top_builddir}/../library/util/libutil.la \
  $(CONFINI_LIBS)
keyboard_SOURCES = \
  rpc/keyboard/attach.c \
  rpc/keyboard/deallocate.c \
  rpc/keyboard/detach.c \
  rpc/keyboard/key.c \
  rpc/init.c \
  keyboard.c \
  keymap.c \
  main.c
keyboard_LDFLAGS = -all-static --static
