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

#include <unistd.h>
#include <sys/fcntl.h>
#include <sys/ioctl.h>
#include "led.h"
#include "../../../../../library/platform/raspi/iomem/libiomem.h"
#include "../../../../../library/platform/raspi/iomem/mailbox.h"

static int fd_iomem;

/**
 * @fn void led_set_activity(led_activity_t)
 * @brief Helper to set activity led state
 * @param state
 */
void led_set_activity( const led_activity_t state ) {
  // allocate pull parameter block
  iomem_gpio_raise_t* raise = malloc( sizeof( iomem_gpio_raise_t ) );
  if ( ! raise ) {
    return;
  }
  // clear parameter blocks
  memset( raise, 0, sizeof( iomem_gpio_pull_t ) );
  // prepare parameter block for pull
  raise->pin = IOMEM_GPIO_ENUM_PIN_CD;
  raise->raise = LED_ACTIVITY_LED_ON == state;
  // handle ioctl error
  if ( -1 == ioctl(  fd_iomem, IOCTL_BUILD_REQUEST( IOMEM_RPC_GPIO_SET_FUNCTION,  sizeof( iomem_gpio_raise_t ), IOCTL_WRONLY ), raise ) ) {
    // free raise
    free( raise );
    // return
    return;
  }
  // free raise
  free( raise );
}

/**
 * @fn bool led_init_status(void)
 * @brief Wrapper to init status led access
 * @return
 */
bool led_init_status( void ) {
  // open iomem device
  fd_iomem = open( IOMEM_DEVICE_PATH, O_RDWR );
  if ( -1 == fd_iomem ) {
    fd_iomem = 0;
    // return error
    return false;
  }
  // allocate function parameter block
  iomem_gpio_function_t* func = malloc( sizeof( iomem_gpio_function_t ) );
  if ( ! func ) {
    // close file descriptor again
    close( fd_iomem );
    // return error
    return false;
  }
  // clear parameter blocks
  memset( func, 0, sizeof( iomem_gpio_function_t ) );
  // change pin 47 to output
  func->pin = IOMEM_GPIO_ENUM_PIN_CD;
  func->function = IOMEM_GPIO_ENUM_FUNCTION_OUTPUT;
  // handle ioctl error
  if ( -1 == ioctl(  fd_iomem, IOCTL_BUILD_REQUEST( IOMEM_RPC_GPIO_SET_FUNCTION,  sizeof( iomem_gpio_function_t ), IOCTL_WRONLY ), func ) ) {
    // close file descriptor again
    close( fd_iomem );
    // free func
    free( func );
    // return error
    return false;
  }
  // free func
  free( func );
  // return success
  return true;
}
