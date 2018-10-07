
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

#ifndef __KERNEL_ARCH_ARM_DEBUG__
#define __KERNEL_ARCH_ARM_DEBUG__

#include <stdint.h>

// FIXME: Move into function
#define BUG( __msg ) ( { \
  asm volatile( \
    "1: .word	0xffffffff\n" \
      ".pushsection .bugtable, \"a\"\n" \
      ".word	%c0\n" \
      ".word	%c1\n" \
      ".word	1b\n" \
      ".word	%c2\n" \
      ".popsection" :: \
      "i"( __FILE__ ), \
      "i"( __LINE__ ), \
      "i"( __msg ) \
  ); \
} )

// FIXME: Move into function
#define BREAKPOINT() asm("swi 3");

#endif
