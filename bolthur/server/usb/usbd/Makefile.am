
AM_CFLAGS = -DPROGRAM_NAME=\"usb/usbd\"

bin_PROGRAMS = usbd
usbd_SOURCES = \
  rpc/attach/device.c \
  rpc/control/message.c \
  rpc/get/description.c \
  rpc/get/descriptor.c \
  rpc/get/roothub.c \
  rpc/handler/register.c \
  rpc/handler/unregister.c \
  rpc/init.c \
  main.c \
  usbd.c
usbd_LDFLAGS = -all-static --static
