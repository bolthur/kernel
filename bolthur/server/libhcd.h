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

#ifndef _LIBHCD_H
#define _LIBHCD_H

#define HCD_DEVICE_PATH "/dev/usb/server/hcd"

#define HCD_SUBMIT_CONTROL_MESSAGE RPC_CUSTOM_START
#define HCD_POLL_INTERRUPT ( HCD_SUBMIT_CONTROL_MESSAGE + 1 )
#define HCD_STOP_TRANSMISSION ( HCD_POLL_INTERRUPT + 1 )

#endif
