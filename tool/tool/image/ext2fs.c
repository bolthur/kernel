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
#include <unistd.h>
#include <sys/types.h>
#include "../image.h"

static errcode_t custom_open(
  [[maybe_unused]] const char* name,
  [[maybe_unused]] int flags,
  io_channel *channel
) {
  io_channel io = malloc( sizeof( struct struct_io_channel ) );
  if ( ! io ) {
    return EXT2_ET_NO_MEMORY;
  }
  memset( io, 0, sizeof( struct struct_io_channel ) );
  io->magic = EXT2_ET_MAGIC_IO_CHANNEL;
  io->manager = &custom_io_manager;
  io->block_size = 1024;
  io->refcount = 1;
  *channel = io;
  return 0;
}

static errcode_t custom_close( io_channel channel ) {
  free( channel );
  return 0;
}

static errcode_t custom_read_blk(
  io_channel channel,
  const unsigned long block,
  const int count,
  void *data
) {
  const off_t block_size = channel->block_size ? channel->block_size : 1024;
  constexpr off_t base_offset = ( off_t )EXT_PARTITION_START * 512;
  const off_t target_offset = base_offset + ( ( off_t )block * block_size );
  // Seek to the target position
  if (fseeko( global_sd_image_fd, target_offset, SEEK_SET ) != 0) {
    return EXT2_ET_SHORT_READ;
  }
  // handle libext2fs negative count logic
  if ( 0 > count ) {
    // negative count means read exactly "-count" bytes
    const size_t bytes_to_read = (size_t)(-count);
    const size_t bytes_read = fread( data, 1, bytes_to_read, global_sd_image_fd );
    if ( bytes_read < bytes_to_read ) {
      return EXT2_ET_SHORT_READ;
    }
  } else {
    // positive count means read "count" number of blocks
    const size_t blocks_read = fread( data, block_size, count, global_sd_image_fd );
    if (blocks_read < (size_t)count) {
      return EXT2_ET_SHORT_READ;
    }
  }
  return 0;
}

static errcode_t custom_write_blk(
  io_channel channel,
  const unsigned long block,
  const int count,
  const void *data
) {
  const off_t block_size = channel->block_size ? channel->block_size : 1024;
  constexpr off_t base_offset = ( off_t )EXT_PARTITION_START * 512;
  const off_t target_offset = base_offset + ( ( off_t )block * block_size );
  // Seek to the target position
  if (fseeko( global_sd_image_fd, target_offset, SEEK_SET ) != 0) {
    return EXT2_ET_SHORT_WRITE;
  }
  if ( 0 > count ) {
    // write exactly "-count" bytes
    const size_t bytes_to_write = ( size_t )( -count );
    const size_t bytes_written = fwrite( data, 1, bytes_to_write, global_sd_image_fd );
    if ( bytes_written < bytes_to_write ) {
      return EXT2_ET_SHORT_WRITE;
    }
  } else {
    // write "count" number of blocks
    const size_t blocks_written = fwrite( data, block_size, count, global_sd_image_fd );
    if ( blocks_written < ( size_t )count ) {
      return EXT2_ET_SHORT_WRITE;
    }
  }
  return 0;
}

static errcode_t custom_flush( [[maybe_unused]] io_channel channel ) {
  fflush( global_sd_image_fd );
  return 0;
}

static errcode_t custom_set_blksize( io_channel channel, const int blksize ) {
  channel->block_size = blksize;
  return 0;
}

struct struct_io_manager custom_io_manager = {
  .magic = EXT2_ET_MAGIC_IO_MANAGER,
  .name = "custom_image_io",
  .open = custom_open,
  .close = custom_close,
  .read_blk = custom_read_blk,
  .write_blk = custom_write_blk,
  .flush = custom_flush,
  .set_blksize = custom_set_blksize
};
