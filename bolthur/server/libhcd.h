/**
 * Copyright (C) 2018 - 2025 bolthur project.
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

#ifndef _LIBHCD_H
#define _LIBHCD_H

#include <sys/bolthur.h>
#include "libusb.h"

#define HCD_DEVICE_PATH "/dev/usb/hcd"

#define HCD_SUBMIT_CONTROL_MESSAGE RPC_CUSTOM_START

typedef struct {
  size_t shm_id;
} hcd_submit_control_message_t;

typedef struct {
  uint32_t device_number;
  uint32_t parent_device_number;
  uint32_t port_number;
  uint32_t last_transfer;
  libusb_transfer_error_t error;
  libusb_pipe_address_t pipe_address;
  libusb_device_request_t request;
  size_t buffer_length;
  size_t timeout;
  uint8_t buffer[];
} hcd_control_message_t;

#endif
