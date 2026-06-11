
AM_CFLAGS = -DPROGRAM_NAME=\"usb/usbd\"

bin_PROGRAMS = usbd
usbd_DEPENDENCIES = \
  ${abs_top_builddir}/../library/util/libutil.la \
  ${abs_top_builddir}/../library/vfs/libvfs.la
usbd_LDADD = \
  ${abs_top_builddir}/../library/util/libutil.la \
  ${abs_top_builddir}/../library/vfs/libvfs.la
usbd_SOURCES = \
  call/attach.c \
  call/check_for_change.c \
  call/child_check_connection.c \
  call/child_detached.c \
  call/child_reset.c \
  call/deallocate.c \
  call/detached.c \
  ioctl/wrapper.c \
  libusbd/address.c \
  libusbd/allocate.c \
  libusbd/attach.c \
  libusbd/configuration.c \
  libusbd/context.c \
  libusbd/control.c \
  libusbd/deallocate.c \
  libusbd/description.c \
  libusbd/descriptor.c \
  libusbd/device.c \
  libusbd/enumerating.c \
  libusbd/handler.c \
  libusbd/init.c \
  libusbd/interrupt.c \
  libusbd/roothub.c \
  libusbd/string.c \
  rpc/attach/device.c \
  rpc/attach/roothub.c \
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
  main.c
usbd_LDFLAGS = -all-static --static
