
AM_CFLAGS = -DPROGRAM_NAME=\"usb/hcd\"

bin_PROGRAMS = hcd
hcd_SOURCES = \
  rpc/interrupt/handle.c \
  rpc/init.c \
  rpc/submit_control_message.c \
  dwhci.c \
  main.c \
  response.c \
  util.c
hcd_LDFLAGS = -all-static --static
