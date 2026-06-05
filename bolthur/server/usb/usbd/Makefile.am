
AM_CFLAGS = -DPROGRAM_NAME=\"usb/usbd\"

bin_PROGRAMS = usbd
usbd_DEPENDENCIES = \
  ${abs_top_builddir}/../library/util/libutil.la \
  ${abs_top_builddir}/../library/vfs/libvfs.la
usbd_LDADD = \
  ${abs_top_builddir}/../library/util/libutil.la \
  ${abs_top_builddir}/../library/vfs/libvfs.la
usbd_SOURCES = \
  libusbd/address.c \
  libusbd/allocate.c \
  libusbd/attach.c \
  libusbd/configuration.c \
  libusbd/control.c \
  libusbd/deallocate.c \
  libusbd/description.c \
  libusbd/descriptor.c \
  libusbd/device.c \
  libusbd/handler.c \
  libusbd/init.c \
  libusbd/interrupt.c \
  libusbd/roothub.c \
  libusbd/string.c \
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
  rpc/interrupt/poll.c \
  rpc/init.c \
  call.c \
  ioctl_wrapper.c \
  main.c
usbd_LDFLAGS = -all-static --static
