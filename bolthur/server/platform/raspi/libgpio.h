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

#if ! defined( _LIBGPIO_H )
#define _LIBGPIO_H

enum iomem_gpio_enum_function {
  IOMEM_GPIO_ENUM_FUNCTION_INPUT = 0x0,
  IOMEM_GPIO_ENUM_FUNCTION_OUTPUT = 0x1,
  IOMEM_GPIO_ENUM_FUNCTION_ALT0 = 0x4,
  IOMEM_GPIO_ENUM_FUNCTION_ALT1 = 0x5,
  IOMEM_GPIO_ENUM_FUNCTION_ALT2 = 0x6,
  IOMEM_GPIO_ENUM_FUNCTION_ALT3 = 0x7,
  IOMEM_GPIO_ENUM_FUNCTION_ALT4 = 0x2,
  IOMEM_GPIO_ENUM_FUNCTION_ALT5 = 0x3,
};
typedef enum  iomem_gpio_enum_function iomem_gpio_enum_function_t;

enum iomem_gpio_enum_pull {
  IOMEM_GPIO_ENUM_NO_PULL = 0x0,
  IOMEM_GPIO_ENUM_PULL_DOWN = 0x1,
  IOMEM_GPIO_ENUM_PULL_UP = 0x2,
};
typedef enum iomem_gpio_enum_pull iomem_gpio_enum_pull_t;

enum iomem_gpio_enum_pin {
  // raspi 3 wifi pin
  #if 3 == RASPI
    IOMEM_GPIO_ENUM_PIN_WIFI0 = 34,
    IOMEM_GPIO_ENUM_PIN_WIFI1 = 35,
    IOMEM_GPIO_ENUM_PIN_WIFI2 = 36,
    IOMEM_GPIO_ENUM_PIN_WIFI3 = 37,
    IOMEM_GPIO_ENUM_PIN_WIFI4 = 38,
    IOMEM_GPIO_ENUM_PIN_WIFI5 = 39,
  #endif
  // emmc gpio pins
  IOMEM_GPIO_ENUM_PIN_CD = 47,
  IOMEM_GPIO_ENUM_PIN_CLK = 48,
  IOMEM_GPIO_ENUM_PIN_CMD = 49,
  IOMEM_GPIO_ENUM_PIN_DAT0 = 50,
  IOMEM_GPIO_ENUM_PIN_DAT1 = 51,
  IOMEM_GPIO_ENUM_PIN_DAT2 = 52,
  IOMEM_GPIO_ENUM_PIN_DAT3 = 53,
};
typedef enum iomem_gpio_enum_pin iomem_gpio_enum_pin_t;

enum iomem_gpio_enum_detect_type {
  IOMEM_GPIO_ENUM_DETECT_TYPE_LOW = 0,
  IOMEM_GPIO_ENUM_DETECT_TYPE_HIGH = 1,
  IOMEM_GPIO_ENUM_DETECT_TYPE_RISING_EDGE = 2,
  IOMEM_GPIO_ENUM_DETECT_TYPE_FALLING_EDGE = 3,
};
typedef enum iomem_gpio_enum_detect_type iomem_gpio_enum_detect_type_t;

#endif
