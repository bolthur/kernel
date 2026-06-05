/**
 * Copyright (C) 2018 - 2026 bolthur project.
 *
 * This file is part of bolthur/kernel.
 *
 * bolthur/kernel is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * bolthur/kernel is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with bolthur/kernel.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "description.h"

/**
 * @fn const char* usbd_description_get(const libusb_device_t*)
 * @brief Get usb description
 * @param dev
 * @return
 */
const char* usbd_description_get( const libusb_device_t* dev ) {
  if ( LIBUSB_DEVICE_STATUS_ATTACHED == dev->status ) {
    return "New device (not ready)";
  }
  if ( LIBUSB_DEVICE_STATUS_POWERED == dev->status ) {
    return "Unknown device (not ready)";
  }
  if ( 1 == dev->number ) {
    return "USB root hub";
  }

  switch ( dev->descriptor.class ) {
    // hubs
    case LIBUSB_DEVICE_CLASS_HUB:
      if ( dev->descriptor.usb_version == 0x210 ) {
        return "USB 2.1 Hub";
      }
      if ( dev->descriptor.usb_version == 0x200 ) {
        return "USB 2.0 Hub";
      }
      if ( dev->descriptor.usb_version == 0x110 ) {
        return "USB 1.1 Hub";
      }
      if ( dev->descriptor.usb_version == 0x100 ) {
        return "USB 1.0 Hub";
      }
      return "USB Hub";
    // vendor specific
    case LIBUSB_DEVICE_CLASS_VENDOR_SPECIFIC:
      if ( dev->descriptor.vendor_id == 0x424 && dev->descriptor.product_id == 0xec00 ) {
        return "SMSC LAN9512";
      }
      // qemu cdc ethernet adapter
      if ( dev->descriptor.vendor_id == 0x525 && dev->descriptor.product_id == 0xa4a2 ) {
        return "QEMU CDC LAN";
      }
      return "Vendor specific";
    // interfaces
    case LIBUSB_DEVICE_CLASS_IN_INTERFACE:
      if ( LIBUSB_DEVICE_STATUS_CONFIGURED == dev->status ) {
        switch ( dev->interfaces[ 0 ].class ) {
          case LIBUSB_INTERFACE_CLASS_AUDIO: return "USB Audio device";
          case LIBUSB_INTERFACE_CLASS_COMMUNICATIONS: return "USB CDC device";
          case LIBUSB_INTERFACE_CLASS_HID:
            switch ( dev->interfaces[ 0 ].protocol ) {
              case 1: return "USB Keyboard";
              case 2: return "USB Mouse";
              default: return "USB HID";
            }
          case LIBUSB_INTERFACE_CLASS_PHYSICAL: return "USB Physical device";
          case LIBUSB_INTERFACE_CLASS_IMAGE: return "USB Imaging device";
          case LIBUSB_INTERFACE_CLASS_PRINTER: return "USB Printer device";
          case LIBUSB_INTERFACE_CLASS_MASS_STORAGE: return "USB Mass storage device";
          case LIBUSB_INTERFACE_CLASS_HUB:
            if ( dev->descriptor.usb_version == 0x210 ) {
              return "USB 2.1 Hub";
            }
            if ( dev->descriptor.usb_version == 0x200 ) {
              return "USB 2.0 Hub";
            }
            if ( dev->descriptor.usb_version == 0x110 ) {
              return "USB 1.1 Hub";
            }
            if ( dev->descriptor.usb_version == 0x100 ) {
              return "USB 1.0 Hub";
            }
            return "USB Hub";
          case LIBUSB_INTERFACE_CLASS_CDC_DATA: return "USB CDC device";
          case LIBUSB_INTERFACE_CLASS_SMART_CARD: return "USB Smart card";
          case LIBUSB_INTERFACE_CLASS_CONTENT_SECURITY: return "USB Content security";
          case LIBUSB_INTERFACE_CLASS_VIDEO: return "USB Video";
          case LIBUSB_INTERFACE_CLASS_PERSONAL_HEALTHCARE: return "USB Personal health care";
          case LIBUSB_INTERFACE_CLASS_AUDIO_VIDEO: return "USB AV device";
          case LIBUSB_INTERFACE_CLASS_DIAGNOSTIC_DEVICE: return "USB Diagnostic device";
          case LIBUSB_INTERFACE_CLASS_WIRELESS_CONTROLLER: return "USB Wireless controller";
          case LIBUSB_INTERFACE_CLASS_MISCELLANEOUS: return "USB Miscellaneous device";
          case LIBUSB_DEVICE_CLASS_VENDOR_SPECIFIC: return "Vendor Specific";
          default: return "Generic device";
        }
      } else {
        return "Unconfigured device";
      }
    default: return "Generic device";
  }
}
