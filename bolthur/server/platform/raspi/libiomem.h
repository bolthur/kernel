/**
 * Copyright (C) 2018 - 2023 bolthur project.
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

typedef enum {
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
  IOMEM_MMIO_ACTION_DMA_READ,
  IOMEM_MMIO_ACTION_DMA_WRITE,
} mmio_action_t;

typedef enum {
  IOMEM_MMIO_SHIFT_NONE = 0,
  IOMEM_MMIO_SHIFT_LEFT,
  IOMEM_MMIO_SHIFT_RIGHT,
} mmio_shift_t;

typedef enum {
  IOMEM_MMIO_SLEEP_NONE = 0,
  IOMEM_MMIO_SLEEP_MILLISECONDS,
  IOMEM_MMIO_SLEEP_SECONDS,
} mmio_sleep_t;

typedef enum {
  IOMEM_MMIO_ABORT_TYPE_NONE = 0,
  IOMEM_MMIO_ABORT_TYPE_TIMEOUT,
  IOMEM_MMIO_ABORT_TYPE_INVALID,
} mmio_abort_type_t;

typedef enum {
  IOMEM_MMIO_FAILURE_CONDITION_OFF = 0,
  IOMEM_MMIO_FAILURE_CONDITION_ON,
} mmio_failure_condition_t;

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
  // only usable for loop
  mmio_failure_condition_t failure_condition;
  uint32_t failure_value;
  // sleep type and amount to sleep
  mmio_sleep_t sleep_type;
  uint32_t sleep;
  // skipped & aborted flag
  mmio_abort_type_t abort_type;
  uint32_t skipped;
  // dma stuff
  uint32_t dma_copy_size;
};
typedef struct iomem_mmio_entry iomem_mmio_entry_t;
typedef struct iomem_mmio_entry iomem_mmio_entry_array_t[];

typedef struct {
  iomem_gpio_enum_pin_t pin;
  iomem_gpio_enum_function_t function;
} iomem_gpio_function_t;

typedef struct iomem_gpio_pull {
  iomem_gpio_enum_pin_t pin;
  iomem_gpio_enum_pull_t pull;
} iomem_gpio_pull_t;

typedef struct {
  iomem_gpio_enum_pin_t pin;
  iomem_gpio_enum_detect_type_t type;
  uint32_t value;
} iomem_gpio_detect_t;

typedef struct iomem_gpio_status {
  iomem_gpio_enum_pin_t pin;
  uint32_t value;
} iomem_gpio_status_t;

typedef struct {
  iomem_gpio_enum_pin_t pin;
  uint32_t value;
} iomem_gpio_event_t;

#endif
