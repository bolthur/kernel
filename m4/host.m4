
AC_DEFUN([BOLTHUR_SET_HOST], [
  AH_TEMPLATE([ELF32], [Define to 1 for 32 bit ELF targets.])
  AH_TEMPLATE([ELF64], [Define to 1 for 64 bit ELF targets.])
  AC_DEFINE([IS_KERNEL], [1], [Define set for libc to compile differently.])
  AH_TEMPLATE([DEBUG], [Set to 1 to enable debug mode.])
  AH_TEMPLATE([KERNEL_DEBUG_PRINT], [Set to 1 to enable kernel output mode.])

  # Test possibe enable debug parameter
  AS_IF([test "x$enable_debug" == "xyes"], [
    CFLAGS="${CFLAGS} -g"
    AC_DEFINE([DEBUG], [1])
  ])

  # Test possibe enable kernel output parameter
  AS_IF([test "x$enable_kernel_print" == "xyes"], [
    CFLAGS="${CFLAGS} -g"
    AC_DEFINE([KERNEL_DEBUG_PRINT], [1])
  ])

  case "${host_cpu}" in
  arm)
    arch_subdir=arm
    host_bfd=elf32-littlearm
    copy_flags="-I ${host_bfd} -O ${host_bfd}"
    output_img=kernel.img
    output_sym=kernel.sym
    cflags_libfdt="-nostdlib -nostartfiles -ffreestanding -Wall -Wextra -Wshadow -std=gnu99 -I. -I./../../lib/stdc/ -Wno-error"
    AC_DEFINE([ARCH_ARM], [1], [Define to 1 for ARM targets.])
    AC_DEFINE([ELF32], [1])

    case "${DEVICE}" in
    rpi1_a)
      CFLAGS="${CFLAGS} -march=armv6zk -mtune=arm1176jzf-s -mfpu=vfp -mfloat-abi=hard"
      subarch_subdir=v6
      vendor_subdir=rpi
      AC_DEFINE([PLATFORM_RPI1_A], [1], [Define to 1 for raspberry pi 1 A platform.])
      AC_DEFINE([ARCH_ARM_V6], [1], [Define to 1 for ARMv6 targets.])
      AC_DEFINE([ARCH_ARM_ARM1176JZF_S], [1], [Define to 1 for ARM ARM1176JZF-S targets.])
      AC_DEFINE([VENDOR_RPI], [1], [Define to 1 for raspberry pi vendor.])
      ;;
    rpi1_a_plus)
      CFLAGS="${CFLAGS} -march=armv6zk -mtune=arm1176jzf-s -mfpu=vfp -mfloat-abi=hard"
      cflags_libfdt="${cflags_libfdt} -march=armv6zk -mtune=arm1176jzf-s -mfpu=vfpv -mfloat-abi=hard"
      subarch_subdir=v6
      vendor_subdir=rpi
      AC_DEFINE([PLATFORM_RPI1_A_PLUS], [1], [Define to 1 for raspberry pi 1 A+ platform.])
      AC_DEFINE([ARCH_ARM_V6], [1], [Define to 1 for ARMv6 targets.])
      AC_DEFINE([ARCH_ARM_ARM1176JZF_S], [1], [Define to 1 for ARM ARM1176JZF-S targets.])
      AC_DEFINE([VENDOR_RPI], [1], [Define to 1 for raspberry pi vendor.])
      ;;
    rpi1_b)
      CFLAGS="${CFLAGS} -march=armv6zk -mtune=arm1176jzf-s -mfpu=vfp -mfloat-abi=hard"
      cflags_libfdt="${cflags_libfdt} -march=armv6zk -mtune=arm1176jzf-s -mfpu=vfpv -mfloat-abi=hard"
      subarch_subdir=v6
      vendor_subdir=rpi
      AC_DEFINE([PLATFORM_RPI1_B], [1], [Define to 1 for raspberry pi 1 B platform.])
      AC_DEFINE([ARCH_ARM_V6], [1], [Define to 1 for ARMv6 targets.])
      AC_DEFINE([ARCH_ARM_ARM1176JZF_S], [1], [Define to 1 for ARM ARM1176JZF-S targets.])
      AC_DEFINE([VENDOR_RPI], [1], [Define to 1 for raspberry pi vendor.])
      ;;
    rpi1_b_plus)
      CFLAGS="${CFLAGS} -march=armv6zk -mtune=arm1176jzf-s -mfpu=vfp -mfloat-abi=hard"
      cflags_libfdt="${cflags_libfdt} -march=armv6zk -mtune=arm1176jzf-s -mfpu=vfpv -mfloat-abi=hard"
      subarch_subdir=v6
      vendor_subdir=rpi
      AC_DEFINE([PLATFORM_RPI1_B_PLUS], [1], [Define to 1 for raspberry pi 1 B+ platform.])
      AC_DEFINE([ARCH_ARM_V6], [1], [Define to 1 for ARMv6 targets.])
      AC_DEFINE([ARCH_ARM_ARM1176JZF_S], [1], [Define to 1 for ARM ARM1176JZF-S targets.])
      AC_DEFINE([VENDOR_RPI], [1], [Define to 1 for raspberry pi vendor.])
      ;;
    rpi2_b)
      CFLAGS="${CFLAGS} -march=armv7-a -mtune=cortex-a7 -mfpu=vfpv4 -mfloat-abi=hard"
      cflags_libfdt="${cflags_libfdt} -march=armv7-a -mtune=cortex-a7 -mfpu=vfpv4 -mfloat-abi=hard"
      subarch_subdir=v7
      vendor_subdir=rpi
      output_img=kernel7.img
      output_sym=kernel7.sym
      AC_DEFINE([ELF32])
      AC_DEFINE([PLATFORM_RPI2_B], [1], [Define to 1 for raspberry pi 2 B platform])
      AC_DEFINE([ARCH_ARM_V7], [1], [Define to 1 for ARMv7 targets.])
      AC_DEFINE([ARCH_ARM_CORTEX_A7], [1], [Define to 1 for ARM Cortex-A7 targets.])
      AC_DEFINE([VENDOR_RPI], [1], [Define to 1 for raspberry pi vendor.])
      ;;
    rpi_zero)
      CFLAGS="${CFLAGS} -march=armv6zk -mtune=arm1176jzf-s -mfpu=vfp -mfloat-abi=hard"
      cflags_libfdt="${cflags_libfdt} -march=armv6zk -mtune=arm1176jzf-s -mfpu=vfpv -mfloat-abi=hard"
      subarch_subdir=v6
      vendor_subdir=rpi
      AC_DEFINE([ELF32])
      AC_DEFINE([PLATFORM_RPI_ZERO], [1], [Define to 1 for raspberry pi 1 platform.])
      AC_DEFINE([ARCH_ARM_V6], [1], [Define to 1 for ARMv6 targets.])
      AC_DEFINE([ARCH_ARM_ARM1176JZF_S], [1], [Define to 1 for ARM ARM1176JZF-S targets.])
      AC_DEFINE([VENDOR_RPI], [1], [Define to 1 for raspberry pi vendor.])
      ;;
    rpi_zero_w)
      CFLAGS="${CFLAGS} -march=armv6zk -mtune=arm1176jzf-s -mfpu=vfp -mfloat-abi=hard"
      cflags_libfdt="${cflags_libfdt} -march=armv6zk -mtune=arm1176jzf-s -mfpu=vfpv -mfloat-abi=hard"
      subarch_subdir=v6
      vendor_subdir=rpi
      AC_DEFINE([PLATFORM_RPI_ZERO_W], [1], [Define to 1 for raspberry pi zero platform.])
      AC_DEFINE([ARCH_ARM_V6], [1], [Define to 1 for ARMv6 targets.])
      AC_DEFINE([ARCH_ARM_ARM1176JZF_S], [1], [Define to 1 for ARM ARM1176JZF-S targets.])
      AC_DEFINE([VENDOR_RPI], [1], [Define to 1 for raspberry pi vendor.])
      ;;
    *)
      AC_MSG_ERROR([unsupported host vendor])
      ;;
    esac
    ;;
  aarch64)
    arch_subdir=arm
    host_bfd=elf64-littleaarch64
    copy_flags="-I ${host_bfd} -O ${host_bfd}"
    AC_DEFINE([ARCH_ARM], [1], [Define to 1 for ARM targets.])
    AC_DEFINE([ELF64], [1])
    case "${DEVICE}" in
    rpi2_b_rev2)
      # -mfpu=fp-armv8 -mfloat-abi=hard
      CFLAGS="${CFLAGS} -march=armv8-a -mtune=cortex-a53"
      cflags_libfdt="${cflags_libfdt} -march=armv8-a -mtune=cortex-a53"
      subarch_subdir=v8
      vendor_subdir=rpi
      # should be may be kernel8
      output_img=kernel7.img
      output_sym=kernel7.sym
      AC_DEFINE([PLATFORM_RPI2_B_REV2], [1], [Define to 1 for raspberry pi 2 B rev. 2 platform])
      AC_DEFINE([ARCH_ARM_V8], [1], [Define to 1 for ARMv7 targets.])
      AC_DEFINE([ARCH_ARM_CORTEX_A53], [1], [Define to 1 for ARM Cortex-A53 targets.])
      AC_DEFINE([VENDOR_RPI], [1], [Define to 1 for raspberry pi vendor.])
      ;;
    rpi3_b)
      # -mfpu=fp-armv8 -mfloat-abi=hard
      CFLAGS="${CFLAGS} -march=armv8-a -mtune=cortex-a53"
      cflags_libfdt="${cflags_libfdt} -march=armv8-a -mtune=cortex-a53"
      subarch_subdir=v8
      vendor_subdir=rpi
      output_img=kernel8.img
      output_sym=kernel8.sym
      AC_DEFINE([PLATFORM_RPI3_B], [1], [Define to 1 for raspberry pi 3 B platform])
      AC_DEFINE([ARCH_ARM_V8], [1], [Define to 1 for ARMv8 targets.])
      AC_DEFINE([ARCH_ARM_CORTEX_A53], [1], [Define to 1 for ARM Cortex-A53 targets.])
      AC_DEFINE([VENDOR_RPI], [1], [Define to 1 for raspberry pi vendor.])
      ;;
    rpi3_b_plus)
      # -mfpu=fp-armv8 -mfloat-abi=hard
      CFLAGS="${CFLAGS} -march=armv8-a -mtune=cortex-a53"
      cflags_libfdt="${cflags_libfdt} -march=armv8-a -mtune=cortex-a53"
      subarch_subdir=v8
      vendor_subdir=rpi
      output_img=kernel8.img
      output_sym=kernel8.sym
      AC_DEFINE([PLATFORM_RPI3_B_PLUS], [1], [Define to 1 for raspberry pi 3 B+ platform])
      AC_DEFINE([ARCH_ARM_V8], [1], [Define to 1 for ARMv8 targets.])
      AC_DEFINE([ARCH_ARM_CORTEX_A53], [1], [Define to 1 for ARM Cortex-A53 targets.])
      AC_DEFINE([VENDOR_RPI], [1], [Define to 1 for raspberry pi vendor.])
      ;;
    *)
      AC_MSG_ERROR([unsupported host vendor])
      ;;
    esac
    ;;
  *)
    AC_MSG_ERROR([unsupported host CPU])
    ;;
  esac
  AC_DEFINE_UNQUOTED([ARCH], [${arch_subdir}], [bolthur/kernel target architecture.])
  AC_DEFINE_UNQUOTED([SUBARCH], [${subarch_subdir}], [bolthur/kernel target subarchitecture.])
  AC_DEFINE_UNQUOTED([VENDOR], [${vendor_subdir}], [bolthur/kernel target vendor.])
  AC_SUBST(arch_subdir)
  AC_SUBST(subarch_subdir)
  AC_SUBST(vendor_subdir)
  AC_SUBST(output_img)
  AC_SUBST(output_sym)
  AC_SUBST(host_bfd)
  AC_SUBST(copy_flags)
  AC_SUBST(cflags_libfdt)
])

AC_DEFUN([BOLTHUR_SET_FLAGS], [
  CFLAGS="${CFLAGS} -ffreestanding -Wall -Wextra -Werror -Wpedantic -Wconversion -std=c18 -nodefaultlibs"
  LDFLAGS="${LDFLAGS} -nostdlib -fno-exceptions"
])

AC_DEFUN([BOLTHUR_PROG_OBJCOPY], [
  AC_CHECK_TOOL([BOLTHUR_OBJCOPY], [objcopy])
  AC_CACHE_CHECK([whether objcopy generates $host_bfd],
    [ac_cv_objcopy_supports_host_bfd],
    [if test "$NOS_OBJCOPY" --info 2>&1 < /dev/null | grep "$host_bfd" > /dev/null; then
      ac_cv_objcopy_supports_host_bfd=no
    else
      ac_cv_objcopy_supports_host_bfd=yes
    fi]
  )
  if test ac_cv_objcopy_supports_host_bfd = no; then
    AC_MSG_ERROR([unsupported host BFD])
  fi
])
