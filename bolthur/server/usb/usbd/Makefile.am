
AM_CFLAGS = -DPROGRAM_NAME=\"usb/usbd\"

bin_PROGRAMS = usbd
usbd_SOURCES = \
  rpc/attach/device.c \
  rpc/control/message.c \
  rpc/get/configuration.c \
  rpc/get/description.c \
  rpc/get/descriptor.c \
  rpc/get/endpoint.c \
  rpc/get/interface.c \
  rpc/get/roothub.c \
  rpc/get/status.c \
  rpc/handler/register.c \
  rpc/handler/unregister.c \
  rpc/init.c \
  call.c \
  main.c \
  usbd.c
usbd_LDFLAGS = -all-static --static
