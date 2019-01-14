
/**
 * Copyright (C) 2017 - 2019 bolthur project.
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

#ifndef __KERNEL_ARCH_ARM_DEBUG__
#define __KERNEL_ARCH_ARM_DEBUG__

#include <stdint.h>

// FIXME: Move into function
#define BUG(__msg) { \
  __asm__ __volatile__("1: .word	0xffffffff\n" \
          ".pushsection .bugtable, \"a\"\n" \
          ".word	%c0\n" \
          ".word	%c1\n" \
          ".word	1b\n" \
          ".word	%c2\n" \
          ".popsection" :: "i"(__FILE__), "i"(__LINE__), \
          "i"(__msg)); \
}

#endif
