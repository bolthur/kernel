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

#ifndef _HUB_H
#define _HUB_H

#include "../../../libusb.h"

#define HUB_ENABLE_DEBUG 1

/**
 * @brief Hub attach context
 */
typedef struct {
  /** attach count */
  uint32_t to_attach;
  /** origin process */
  pid_t origin;
  /** data info */
  size_t data_info;
  /** hub data */
  libusb_hub_device_t* hub;
  /** root hub device number */
  uint32_t roothub;
  /** last port checked */
  uint32_t port_number;
  /** hub device number */
  uint32_t device_number;
} hub_attach_context_t;

/**
 * @brief Hub detach context
 */
typedef struct {
  /** detach count */
  size_t to_detach;
  /** index detach is running for */
  uint32_t idx;
  /** hub data */
  libusb_hub_device_t* hub;
  /** origin process */
  pid_t origin;
  /** data info */
  size_t data_info;
} hub_detach_context_t;

typedef enum {
  HUB_CHECK_CHANGE_STATUS_ATTACH = 0,
  HUB_CHECK_CHANGE_STATUS_DETACH = 1,
  HUB_CHECK_CHANGE_STATUS_CASCADE = 2,
} hub_check_change_status_t;

/**
 * @brief Hub change check context
 */
typedef struct {
  /** array of entries to check */
  bool* to_check;
  /** index change is running for */
  uint32_t idx;
  /** hub data */
  libusb_hub_device_t* hub;
  /** last status */
  hub_check_change_status_t status;
  /** origin process */
  pid_t origin;
  /** data info */
  size_t data_info;
} hub_check_change_context_t;

void hub_append( libusb_hub_device_t* );
void hub_detach( const libusb_hub_device_t* );
void hub_destroy( libusb_hub_device_t* );
libusb_hub_device_t* hub_get( uint32_t );
int hub_read_descriptor( uint32_t, void** );
int hub_get_status( uint32_t, libusb_hub_device_t* );
int hub_change_port_feature( uint32_t, libusb_hub_port_feature_t, uint8_t, bool );
int hub_power_on( uint32_t, const libusb_hub_device_t* );
int hub_get_port_status( uint32_t, libusb_hub_device_t*, uint8_t );
int hub_port_reset( uint32_t, libusb_hub_device_t*, uint8_t );
int hub_port_connection_changed( uint32_t, libusb_hub_device_t*, uint8_t, hub_attach_context_t* );
int hub_check_connection( uint32_t, libusb_hub_device_t*, uint8_t, hub_attach_context_t* );
int hub_perform_detach( libusb_hub_device_t*, size_t, pid_t, size_t );

#endif
