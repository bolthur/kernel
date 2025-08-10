
AM_CFLAGS = -DPROGRAM_NAME=\"usb/device/hid/keyboard\"

bin_PROGRAMS = keyboard
keyboard_DEPENDENCIES = \
  ${abs_top_builddir}/../library/hid/libhid.la \
  ${abs_top_builddir}/../library/usb/libusb.la
keyboard_LDADD = \
  ${abs_top_builddir}/../library/hid/libhid.la \
  ${abs_top_builddir}/../library/usb/libusb.la
keyboard_SOURCES = \
  rpc/keyboard/attach.c \
  rpc/keyboard/deallocate.c \
  rpc/keyboard/detach.c \
  rpc/init.c \
  keyboard.c \
  keymap.c \
  main.c
keyboard_LDFLAGS = -all-static --static
