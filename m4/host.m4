
AC_DEFUN([BOLTHUR_SET_HOST], [
  AH_TEMPLATE([ELF32], [Define to 1 for 32 bit ELF targets])
  AH_TEMPLATE([ELF64], [Define to 1 for 64 bit ELF targets])
  AH_TEMPLATE([DEBUG], [Set to 1 to enable debug mode])
  AH_TEMPLATE([SMP_ENABLED], [Define to 1 for SMP capable hosts])
  AH_TEMPLATE([SMP_CORE_NUMBER], [Define to amount of smp cores])
  AH_TEMPLATE([FPU_ENABLED], [Define to 1 for host with hardware FPU])
  AH_TEMPLATE([IS_HIGHER_HALF], [Define to 1 when kernel is higher half])
  AH_TEMPLATE([ARCH_ARM], [Define to 1 for ARM targets])
  AH_TEMPLATE([VENDOR_RPI], [Define to 1 for raspberry pi vendor])
  AH_TEMPLATE([SERIAL_TTY], [Define to 1 for output via serial])
  AH_TEMPLATE([KERNEL_PRINT], [Define to 1 to enable kernel print])
  AH_TEMPLATE([PRINT_MM_PHYS], [Define to 1 to enable output of physical memory manager])
  AH_TEMPLATE([PRINT_MM_VIRT], [Define to 1 to enable output of virtual memory manager])
  AH_TEMPLATE([PRINT_MM_HEAP], [Define to 1 to enable output of kernel heap])
  AH_TEMPLATE([PRINT_MAILBOX], [Define to 1 to enable output of mailbox])

  # Define for kernel mode
  AC_DEFINE([IS_KERNEL], [1], [Define set for libc to compile differently])

  # Test possibe enable debug parameter
  AS_IF([test "x$enable_debug" == "xyes"], [
    CFLAGS="${CFLAGS} -g"
    AC_DEFINE([DEBUG], [1])
  ])

  # Test possible serial tty activation
  AS_IF([test "x$enable_serial_tty" == "xyes"], [
    AC_DEFINE([SERIAL_TTY], [1])
  ])

  # Test for general output enable
  AS_IF([test "x$enable_output" == "xyes"], [
    AC_DEFINE([KERNEL_PRINT], [1])
  ])

  # Test for physical memory manager output
  AS_IF([test "x$enable_output_mm_phys" == "xyes"], [
    AC_DEFINE([PRINT_MM_PHYS], [1])
  ])

  # Test for virtual memory manager output
  AS_IF([test "x$enable_output_mm_virt" == "xyes"], [
    AC_DEFINE([PRINT_MM_VIRT], [1])
  ])

  # Test for kernel heap output
  AS_IF([test "x$enable_output_mm_heap" == "xyes"], [
    AC_DEFINE([PRINT_MM_HEAP], [1])
  ])

  # Test for mailbox output
  AS_IF([test "x$enable_output_mailbox" == "xyes"], [
    AC_DEFINE([PRINT_MAILBOX], [1])
  ])

  case "${host_cpu}" in
  arm)
    arch_subdir=arm
    host_bfd=elf32-littlearm
    output_img=kernel.img
    output_sym=kernel.sym
    executable_format=32
    AC_DEFINE([ARCH_ARM], [1])
    AC_DEFINE([ELF32], [1])

    case "${DEVICE}" in
    rpi2_b_rev1)
      CFLAGS="${CFLAGS} -march=armv7-a -mtune=cortex-a7 -mfpu=neon-vfpv3 -mfloat-abi=hard"
      subarch_subdir=v7
      vendor_subdir=rpi
      output_img=kernel7.img
      output_sym=kernel7.sym
      AC_DEFINE([ELF32])
      AC_DEFINE([PLATFORM_RPI2_B], [1], [Define to 1 for raspberry pi 2 B platform])
      AC_DEFINE([ARCH_ARM_V7], [1], [Define to 1 for ARMv7 targets])
      AC_DEFINE([ARCH_ARM_CORTEX_A7], [1], [Define to 1 for ARM Cortex-A7 targets])
      AC_DEFINE([VENDOR_RPI], [1])
      AC_DEFINE([SMP_ENABLED], [1])
      AC_DEFINE([SMP_CORE_NUMBER], [4])
      AC_DEFINE([FPU_ENABLED], [1])
      AC_DEFINE([IS_HIGHER_HALF], [1])
      ;;
    rpi_zero_w)
      CFLAGS="${CFLAGS} -march=armv6zk -mtune=arm1176jzf-s -mfpu=vfpv2 -mfloat-abi=hard"
      subarch_subdir=v6
      vendor_subdir=rpi
      AC_DEFINE([PLATFORM_RPI_ZERO_W], [1], [Define to 1 for raspberry pi zero platform])
      AC_DEFINE([ARCH_ARM_V6], [1], [Define to 1 for ARMv6 targets])
      AC_DEFINE([ARCH_ARM_ARM1176JZF_S], [1], [Define to 1 for ARM ARM1176JZF-S targets])
      AC_DEFINE([VENDOR_RPI], [1])
      AC_DEFINE([FPU_ENABLED], [1])
      AC_DEFINE([IS_HIGHER_HALF], [1])
      ;;
    rpi3_b)
      CFLAGS="${CFLAGS} -march=armv8-a -mtune=cortex-a53 -mfpu=neon-vfpv4 -mfloat-abi=hard"
      subarch_subdir=v8
      vendor_subdir=rpi
      output_img=kernel8.img
      output_sym=kernel8.sym
      AC_DEFINE([PLATFORM_RPI3_B], [1], [Define to 1 for raspberry pi 3 B platform])
      AC_DEFINE([ARCH_ARM_V8], [1], [Define to 1 for ARMv8 targets])
      AC_DEFINE([ARCH_ARM_CORTEX_A53], [1], [Define to 1 for ARM Cortex-A53 targets])
      AC_DEFINE([VENDOR_RPI], [1])
      AC_DEFINE([SMP_ENABLED], [1])
      AC_DEFINE([SMP_CORE_NUMBER], [4])
      AC_DEFINE([FPU_ENABLED], [1])
      AC_DEFINE([IS_HIGHER_HALF], [1])
      ;;
    *)
      AC_MSG_ERROR([unsupported host vendor])
      ;;
    esac
    ;;
  aarch64)
    arch_subdir=arm
    host_bfd=elf64-littleaarch64
    executable_format=32
    AC_DEFINE([ARCH_ARM], [1])
    AC_DEFINE([ELF64], [1])

    case "${DEVICE}" in
    rpi3_b)
      CFLAGS="${CFLAGS} -march=armv8-a -mtune=cortex-a53"
      subarch_subdir=v8
      vendor_subdir=rpi
      output_img=kernel8.img
      output_sym=kernel8.sym
      AC_DEFINE([PLATFORM_RPI3_B], [1], [Define to 1 for raspberry pi 3 B platform])
      AC_DEFINE([ARCH_ARM_V8], [1], [Define to 1 for ARMv8 targets])
      AC_DEFINE([ARCH_ARM_CORTEX_A53], [1], [Define to 1 for ARM Cortex-A53 targets])
      AC_DEFINE([VENDOR_RPI], [1])
      AC_DEFINE([SMP_ENABLED], [1])
      AC_DEFINE([SMP_CORE_NUMBER], [4])
      AC_DEFINE([FPU_ENABLED], [1])
      AC_DEFINE([IS_HIGHER_HALF], [1])
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

  copy_flags="-I ${host_bfd} -O ${host_bfd}"

  AC_DEFINE_UNQUOTED([ARCH], [${arch_subdir}], [bolthur/kernel target architecture])
  AC_DEFINE_UNQUOTED([SUBARCH], [${subarch_subdir}], [bolthur/kernel target subarchitecture])
  AC_DEFINE_UNQUOTED([VENDOR], [${vendor_subdir}], [bolthur/kernel target vendor])
  AC_SUBST(arch_subdir)
  AC_SUBST(subarch_subdir)
  AC_SUBST(vendor_subdir)
  AC_SUBST(output_img)
  AC_SUBST(output_sym)
  AC_SUBST(host_bfd)
  AC_SUBST(copy_flags)
  AC_SUBST(executable_format)
])

AC_DEFUN([BOLTHUR_SET_FLAGS], [
  CFLAGS="${CFLAGS} -ffreestanding -Wall -Wextra -Werror -Wpedantic -Wconversion -nodefaultlibs -std=c18"
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
