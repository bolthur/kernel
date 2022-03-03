/**
 * Copyright (C) 2018 - 2022 bolthur project.
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

#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <errno.h>
#include <sys/bolthur.h>
#include "libgpio.h"

#if ! defined( _LIBIOMEM_H )
#define _LIBIOMEM_H

#define IOMEM_RPC_MAILBOX RPC_CUSTOM_START
#define IOMEM_RPC_MMIO_PERFORM IOMEM_RPC_MAILBOX + 1
#define IOMEM_RPC_MMIO_LOCK IOMEM_RPC_MMIO_PERFORM + 1
#define IOMEM_RPC_MMIO_UNLOCK IOMEM_RPC_MMIO_LOCK + 1
#define IOMEM_RPC_GPIO_SET_FUNCTION IOMEM_RPC_MMIO_UNLOCK + 1
#define IOMEM_RPC_GPIO_SET_PULL IOMEM_RPC_GPIO_SET_FUNCTION + 1
#define IOMEM_RPC_GPIO_SET_DETECT IOMEM_RPC_GPIO_SET_PULL + 1
#define IOMEM_RPC_GPIO_STATUS IOMEM_RPC_GPIO_SET_DETECT + 1
#define IOMEM_RPC_GPIO_EVENT IOMEM_RPC_GPIO_STATUS + 1
#define IOMEM_RPC_GPIO_LOCK IOMEM_RPC_GPIO_EVENT + 1
#define IOMEM_RPC_GPIO_UNLOCK IOMEM_RPC_GPIO_LOCK + 1

#define IOMEM_DEVICE_PATH "/dev/iomem"

enum mmio_action {
  IOMEM_MMIO_ACTION_LOOP_EQUAL = 1,
  IOMEM_MMIO_ACTION_LOOP_NOT_EQUAL,
  IOMEM_MMIO_ACTION_LOOP_TRUE,
  IOMEM_MMIO_ACTION_LOOP_FALSE,
  IOMEM_MMIO_ACTION_READ,
  IOMEM_MMIO_ACTION_READ_OR,
  IOMEM_MMIO_ACTION_READ_AND,
  IOMEM_MMIO_ACTION_WRITE,
  IOMEM_MMIO_ACTION_WRITE_PREVIOUS_READ,
  IOMEM_MMIO_ACTION_WRITE_OR_PREVIOUS_READ,
  IOMEM_MMIO_ACTION_WRITE_AND_PREVIOUS_READ,
  IOMEM_MMIO_ACTION_DELAY,
  IOMEM_MMIO_ACTION_SLEEP,
  IOMEM_MMIO_ACTION_SYNC_BARRIER,
};
typedef enum mmio_action mmio_action_t;

enum mmio_shift {
  IOMEM_MMIO_SHIFT_NONE = 0,
  IOMEM_MMIO_SHIFT_LEFT,
  IOMEM_MMIO_SHIFT_RIGHT,
};
typedef enum mmio_shift mmio_shift_t;

enum mmio_sleep {
  IOMEM_MMIO_SLEEP_NONE = 0,
  IOMEM_MMIO_SLEEP_MILLISECONDS,
  IOMEM_MMIO_SLEEP_SECONDS,
};
typedef enum mmio_sleep mmio_sleep_t;

enum mmio_abort_type {
  IOMEM_MMIO_ABORT_TYPE_NONE = 0,
  IOMEM_MMIO_ABORT_TYPE_TIMEOUT,
  IOMEM_MMIO_ABORT_TYPE_INVALID,
};
typedef enum mmio_abort_type mmio_abort_type_t;

struct iomem_mmio_entry {
  // action type
  mmio_action_t type;
  // memory offset
  uint32_t offset;
  // value ( usage depending on action type )
  uint32_t value;
  // shift type and shift bits to be applied
  mmio_shift_t shift_type;
  uint32_t shift_value;
  // value to be used for loop check
  uint32_t loop_and;
  uint32_t loop_max_iteration;
  // sleep type and amount to sleep
  mmio_sleep_t sleep_type;
  uint32_t sleep;
  // skipped & aborted flag
  mmio_abort_type_t abort_type;
  uint32_t skipped;
};
typedef struct iomem_mmio_entry iomem_mmio_entry_t;
typedef struct iomem_mmio_entry* iomem_mmio_entry_ptr_t;
typedef struct iomem_mmio_entry iomem_mmio_entry_array_t[];

struct iomem_gpio_function {
  iomem_gpio_enum_pin_t pin;
  iomem_gpio_enum_function_t function;
};
typedef struct iomem_gpio_function iomem_gpio_function_t;
typedef struct iomem_gpio_function* iomem_gpio_function_ptr_t;

struct iomem_gpio_pull {
  iomem_gpio_enum_pin_t pin;
  iomem_gpio_enum_pull_t pull;
};
typedef struct iomem_gpio_pull iomem_gpio_pull_t;
typedef struct iomem_gpio_pull* iomem_gpio_pull_ptr_t;

struct iomem_gpio_detect {
  iomem_gpio_enum_pin_t pin;
  iomem_gpio_enum_detect_type_t type;
  uint32_t value;
};
typedef struct iomem_gpio_detect iomem_gpio_detect_t;
typedef struct iomem_gpio_detect* iomem_gpio_detect_ptr_t;

struct iomem_gpio_status {
  iomem_gpio_enum_pin_t pin;
  uint32_t value;
};
typedef struct iomem_gpio_status iomem_gpio_status_t;
typedef struct iomem_gpio_status* iomem_gpio_status_ptr_t;

struct iomem_gpio_event {
  iomem_gpio_enum_pin_t pin;
  uint32_t value;
};
typedef struct iomem_gpio_event iomem_gpio_event_t;
typedef struct iomem_gpio_event* iomem_gpio_event_ptr_t;

#endif
