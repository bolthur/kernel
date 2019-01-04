
/**
 * mist-system/kernel
 * Copyright (C) 2017 - 2018 mist-system project.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __KERNEL_DEBUG__
#define __KERNEL_DEBUG__

#include <stdint.h>

// FIXME: Remove, when defines have been written to functions
#if defined( ARCH_ARM )
  #include <_arch/_arm/debug.h>
#else
  #error "Debug defines not available"
#endif

#define BUG_ON( cond, msg ) ( cond ? BUG( msg ) : ( void )0 );

#if defined( __cplusplus )
extern "C" {
#endif

typedef struct {
  const char *filename;
  uint32_t line;
  uint64_t addr;
  const char *msg;
} bug_entry_t;

void debug_init( void );
void debug_breakpoint( void );

#if defined( __cplusplus )
}
#endif

#endif
