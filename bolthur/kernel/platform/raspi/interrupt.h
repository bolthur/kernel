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

#ifndef _PLATFORM_RASPI_INTERRUPT_H
#define _PLATFORM_RASPI_INTERRUPT_H

// undocumented irq
#define IRQ_USB 9
// documented irq
#define IRQ_MAILBOX 1
#define IRQ_AUX 29
#define IRQ_I2C_SPI 43
#define IRQ_PWA0 45
#define IRQ_PWA1 46
#define IRQ_SMI 48
#define IRQ_GPIO0 49
#define IRQ_GPIO1 50
#define IRQ_GPIO2 51
#define IRQ_GPIO3 52
#define IRQ_I2C 53
#define IRQ_SPI 54
#define IRQ_PCM 55
#define IRQ_UART 57

#endif
