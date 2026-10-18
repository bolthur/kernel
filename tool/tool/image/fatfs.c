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

#define _DEFAULT_SOURCE
#include <stdio.h>
#include <time.h>
#include <unistd.h>
#include <sys/types.h>
#include <ff.h>
#include <diskio.h>
#include "../image.h"

PARTITION VolToPart[] = { { .pd = 0, .pt = 1, } };

DSTATUS disk_status( [[maybe_unused]] BYTE pdrv ) {
  return 0;
}

DSTATUS disk_initialize( [[maybe_unused]] BYTE pdrv ) {
  return 0;
}

DRESULT disk_read( [[maybe_unused]] BYTE pdrv, BYTE* buff, LBA_t sector, UINT count ) {
  const off_t offset = ( off_t )sector * SECTOR_SIZE;
  fseeko( global_sd_image_fd, offset, SEEK_SET );
  fread( buff, SECTOR_SIZE, count, global_sd_image_fd );
  return RES_OK;
}

DRESULT disk_write( [[maybe_unused]] BYTE pdrv, const BYTE* buff, LBA_t sector, UINT count ) {
  const off_t offset = ( off_t )sector * SECTOR_SIZE;
  fseeko( global_sd_image_fd, offset, SEEK_SET );
  fwrite( buff, SECTOR_SIZE, count, global_sd_image_fd );
  return RES_OK;
}

DRESULT disk_ioctl( [[maybe_unused]] BYTE pdrv, BYTE cmd, [[maybe_unused]] void* buff ) {
  switch (cmd) {
    case CTRL_SYNC:
      fflush( global_sd_image_fd );
      return RES_OK;
    case CTRL_EJECT:
      return RES_OK;
    case GET_SECTOR_COUNT:
      *( LBA_t* )buff = FAT_PARTITION_SIZE;
      return RES_OK;
    case GET_SECTOR_SIZE:
      *( WORD* )buff = SECTOR_SIZE;
      return RES_OK;
    case GET_BLOCK_SIZE:
      *( DWORD* )buff = 8;
      return RES_OK;
  }
  return RES_PARERR;
}

DWORD get_fattime( void ) {
  const time_t t = time( nullptr );
  struct tm* tm_info = localtime( &t );
  if ( ! tm_info ) {
    return ( ( DWORD )( 2026 - 1980 ) << 25 ) | ( ( DWORD )1 << 21 ) | ( ( DWORD )1 << 16 );
  }
  return ( ( DWORD )( tm_info->tm_year + 1900 - 1980 ) << 25 )
     | ( ( DWORD )( tm_info->tm_mon + 1 ) << 21 )
     | ( ( DWORD )tm_info->tm_mday << 16 )
     | ( ( DWORD )tm_info->tm_hour << 11 )
     | ( ( DWORD )tm_info->tm_min << 5 )
     | ( ( DWORD )( tm_info->tm_sec / 2 ) );
}
