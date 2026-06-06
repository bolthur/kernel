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

#ifndef _LIBUSBD_INIT_H
#define _LIBUSBD_INIT_H

#include "../../libusbd.h"

//#define USBD_ENABLE_DEBUG 1
//#define USBD_ENABLE_ERROR 1

#define CONTROL_MESSAGE_TIMEOUT 10

extern int fd_hcd;
extern libusb_device_t* head;

int usbd_init( void );

#endif
