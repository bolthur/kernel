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

#include <libgen.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include <sys/bolthur.h>
#include "../global.h"
#include "../rpc.h"
#include "../sd.h"
#include "../../../../../library/fs/mbr.h"
#include "../../../../../library/fs/fat.h"
#include "../../../../../library/fs/ext.h"


static void ls_ext( ext_fs_t* fs, const char* path ) {
  // variable for inode
  ext_inode_t inode;
  // try to read inode
  if ( ! ext_directory_get_inode( fs, path, &inode ) ) {
    return;
  }
  // handle no dir
  if ( EXT_S_IFDIR != ( inode.inode->i_mode & 0xF000 ) ) {
    return;
  }
  ext_directory_entry_t* entry = NULL;
  // allocate buffer
  uint8_t* buffer = malloc( inode.inode->i_size );
  if ( ! buffer ) {
    return;
  }
  // clear buffer
  memset( buffer, 0, inode.inode->i_size );
  // read inode data
  if ( ! ext_inode_read_data( &inode, 0, inode.inode->i_size, buffer ) ) {
    free( buffer );
    return;
  }
  // loop while position is smaller than size
  uint32_t position = 0;
  while ( position < inode.inode->i_size ) {
    // get entry from buffer
    entry = ( ext_directory_entry_t* )( &buffer[ position ] );
    // increment position
    position += entry->rec_len;
    // handle unknown
    if ( entry->inode == EXT_FT_UNKNOWN ) {
      continue;
    }

    char* name = strdup( entry->name );
    if ( ! name ) {
      break;
    }
    // read inode of file
    ext_inode_t tmp_inode;
    if ( ! ext_inode_read_inode( fs, entry->inode, &tmp_inode ) ) {
      free( name );
      break;
    }
    EARLY_STARTUP_PRINT( "%"PRIu32" %s\r\n", tmp_inode.inode->i_size, name )
    if ( ! ext_inode_release_inode( &tmp_inode ) ) {
      free( name );
      break;
    }
    free( name );

    // handle invalid length
    if ( 0 == entry->rec_len ) {
      EARLY_STARTUP_PRINT( "Invalid record length!\r\n" )
      break;
    }
  }
  // free buffer
  free( buffer );
  ext_inode_release_inode( &inode );
}

static void ls_fat( fat_fs_t* fs, const char* path ) {
  fat_directory_t* dir = malloc( sizeof( *dir ) );
  if ( ! dir ) {
    return;
  }
  fat_directory_entry_t* entry = malloc( sizeof( *entry ) );
  if ( ! entry ) {
    free( dir );
    return;
  }
  // open directory
  if ( ! fat_directory_open( fs, path, dir ) ) {
    EARLY_STARTUP_PRINT(
      "Unable to open directory \"%s\": %s\r\n",
      path,
      strerror( errno )
    )
    fat_directory_close( dir );
    free( dir );
    free( entry );
    return;
  }
  // loop through entries
  // variables for accessing entries
  fat_extract_t extract;
  uint32_t idx = 0;
  // loop while there is something to loop
  while (
    FAT_EXTRACT_END != ( extract = fat_directory_extract( dir, entry, &idx ) )
  ) {
    // handle possible error
    if ( FAT_EXTRACT_ERROR == extract ) {
      fat_directory_close( dir );
      free( dir );
      free( entry );
      return;
    }
    // skip unused
    if ( FAT_EXTRACT_SKIP == extract ) {
      continue;
    }
    // print name and size
    EARLY_STARTUP_PRINT( "%"PRIu32" %s\r\n", entry->size, entry->name )
  }
  /*for ( uint32_t idx = 0; idx < dir->entry_count; idx++ ) {
    fat_extract_t extract = fat_directory_extract(
      dir, idx, entry, &idx );
    // handle error
    if ( FAT_EXTRACT_ERROR == extract ) {
      fat_directory_close( dir );
      free( dir );
      free( entry );
      return;
    // handle skip
    } else if ( FAT_EXTRACT_UNUSED == extract ) {
      continue;
    // handle end reached
    } else if ( FAT_EXTRACT_END == extract ) {
      break;
    }
    // print name and size
    EARLY_STARTUP_PRINT( "%"PRIu32" %s\r\n", entry->size, entry->name )
  }*/
  // close directory again
  fat_directory_close( dir );
  free( dir );
  free( entry );
}


/**
 * @fn void rpc_handle_mount(size_t, pid_t, size_t, size_t)
 * @brief Handle mount request
 *
 * @param type
 * @param origin
 * @param data_info
 * @param response_info
 */
void rpc_handle_mount(
  __unused size_t type,
  __unused pid_t origin,
  __unused size_t data_info,
  __unused size_t response_info
) {
  EARLY_STARTUP_PRINT( "fooo\r\n" )
  vfs_mount_response_t response = { .result = -EAGAIN };
  vfs_mount_request_t* request = malloc( sizeof( *request ) );
  if ( ! request ) {
    bolthur_rpc_return( type, &response, sizeof( response ), NULL );
    return;
  }
  EARLY_STARTUP_PRINT( "fooo\r\n" )
  // clear variables
  memset( request, 0, sizeof( *request ) );
  // handle no data
  if( ! data_info ) {
    bolthur_rpc_return( type, &response, sizeof( response ), NULL );
    free( request );
    return;
  }
  EARLY_STARTUP_PRINT( "fooo\r\n" )
  // fetch rpc data
  _syscall_rpc_get_data( request, sizeof( *request ), data_info, false );
  // handle error
  if ( errno ) {
    response.result = -errno;
    bolthur_rpc_return( type, &response, sizeof( response ), NULL );
    free( request );
    return;
  }
  EARLY_STARTUP_PRINT( "fooo\r\n" )
  // extract partition
  int32_t partition_number = mbr_extract_partition_from_path( request->source );
  if ( 0 > partition_number ) {
    response.result = partition_number;
    bolthur_rpc_return( type, &response, sizeof( response ), NULL );
    free( request );
    return;
  }
  EARLY_STARTUP_PRINT( "fooo\r\n" )
  // handle partition number to big
  if ( partition_number > PARTITION_TABLE_NUMBER ) {
    response.result = -EINVAL;
    bolthur_rpc_return( type, &response, sizeof( response ), NULL );
    free( request );
    return;
  }
  EARLY_STARTUP_PRINT( "fooo\r\n" )
  EARLY_STARTUP_PRINT( "request->type = %s\r\n", request->type )
  // transform type to constant
  int32_t partition_type = mbr_filesystem_to_type( request->type );
  EARLY_STARTUP_PRINT( "partition_type = %"PRIx32"\r\n", partition_type )
  if ( 0 > partition_type ) {
    response.result = partition_type;
    bolthur_rpc_return( type, &response, sizeof( response ), NULL );
    free( request );
    return;
  }
  EARLY_STARTUP_PRINT( "fooo\r\n" )
  // get mbr table entry
  mbr_table_entry_t* entry = ( mbr_table_entry_t* )(
    mbr_data + PARTITION_TABLE_OFFSET
      + ( ( size_t )partition_number * sizeof( mbr_table_entry_t ) )
  );
  // check partition type to match
  if ( entry->data.system_id != ( uint8_t )partition_type ) {
    response.result = -EINVAL;
    bolthur_rpc_return( type, &response, sizeof( response ), NULL );
    free( request );
    return;
  }
  EARLY_STARTUP_PRINT( "fooo\r\n" )

  // print mbr information
  uint16_t start_sector = entry->data.start_sector;
  uint16_t start_cylinder = entry->data.start_cylinder;
  uint16_t end_sector = entry->data.end_sector;
  uint16_t end_cylinder = entry->data.end_cylinder;
  EARLY_STARTUP_PRINT(
    "bootable = %"PRIu8", start_head = %"PRIu8", start_sector = %"PRIu16", "
    "start_cylinder = %"PRIu16", system_id = %"PRIu8", end_head = %"PRIu8", "
    "end_sector = %"PRIu16", end_cylinder = %"PRIu16", relative_sector = %"PRIu32", "
    "total_sector = %"PRIu32"\r\n",
    entry->data.bootable,
    entry->data.start_head,
    start_sector,
    start_cylinder,
    entry->data.system_id,
    entry->data.end_head,
    end_sector,
    end_cylinder,
    entry->data.relative_sector,
    entry->data.total_sector
  );

  uint32_t sector_size = sd_device_block_size();
  if ( partition_type == PARTITION_TYPE_LINUX_NATIVE ) {
    // setup ext device data
    ext_fs_t* fs_data = ext_fs_init(
      sd_read_block,
      sd_write_block,
      entry->data.relative_sector * sector_size
    );
    if ( ! fs_data ) {
      response.result = -errno;
      bolthur_rpc_return( type, &response, sizeof( response ), NULL );
      free( request );
      return;
    }
    // mount stuff
    if ( ! ext_fs_mount( fs_data ) ) {
      response.result = -errno;
      bolthur_rpc_return( type, &response, sizeof( response ), NULL );
      free( request );
      return;
    }
    // debug list root
    EARLY_STARTUP_PRINT( "listing content of /etc\r\n" )
    ls_ext( fs_data, "/etc" );
    EARLY_STARTUP_PRINT( "listing content of /\r\n" )
    ls_ext( fs_data, "/" );
    EARLY_STARTUP_PRINT( "listing content of /server\r\n" )
    ls_ext( fs_data, "/server" );
  } else if ( partition_type == PARTITION_TYPE_FAT32_LBA ) {
    // setup ext device data
    fat_fs_t* fs_data = fat_fs_init(
      sd_read_block,
      sd_write_block,
      entry->data.relative_sector * sector_size
    );
    if ( ! fs_data ) {
      response.result = -errno;
      bolthur_rpc_return( type, &response, sizeof( response ), NULL );
      free( request );
      return;
    }
    // mount stuff
    if ( ! fat_fs_mount( fs_data ) ) {
      response.result = -errno;
      bolthur_rpc_return( type, &response, sizeof( response ), NULL );
      free( request );
      return;
    }
    // debug list root
    EARLY_STARTUP_PRINT( "listing content of /\r\n" )
    ls_fat( fs_data, "/foobarlongfolder/foo" );
    ls_fat( fs_data, "/foobarlongfolder" );
    ls_fat( fs_data, "/" );
  }

  // debug output
  EARLY_STARTUP_PRINT(
    "partition %"PRId32" has type %#"PRIx8"\r\n",
    partition_number, entry->data.system_id
  )
  EARLY_STARTUP_PRINT( "request->source = %s\r\n", request->source )
  EARLY_STARTUP_PRINT( "request->target = %s\r\n", request->target )
  EARLY_STARTUP_PRINT( "request->type = %s\r\n", request->type )
  EARLY_STARTUP_PRINT( "request->flags = %lu\r\n", request->flags )


  bolthur_rpc_return( type, &response, sizeof( response ), NULL );
  free( request );
}
