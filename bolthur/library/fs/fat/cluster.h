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

#ifndef _FAT_CLUSTER_H
#define _FAT_CLUSTER_H

#define FAT_FAT32_BAD_CLUSTER 0xFF7
#define FAT_FAT32_CLUSTER_CHAIN_END 0xFF8

#define FAT_FAT16_BAD_CLUSTER 0xFFF7
#define FAT_FAT16_CLUSTER_CHAIN_END 0xFFF8

#define FAT_FAT32_BAD_CLUSTER 0x0FFFFFF7
#define FAT_FAT32_CLUSTER_CHAIN_END 0x0FFFFFF8

#define FAT_EXFAT_BAD_CLUSTER 0xFFFFFFF7
#define FAT_EXFAT_CLUSTER_CHAIN_END 0xFFFFFFF8

#endif
