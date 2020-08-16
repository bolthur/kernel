
AC_DEFUN([BOLTHUR_KERNEL_SET_HOST], [
  # General define templates
  AH_TEMPLATE([ELF32], [Define to 1 for 32 bit ELF targets])
  AH_TEMPLATE([ELF64], [Define to 1 for 64 bit ELF targets])
  AH_TEMPLATE([IS_HIGHER_HALF], [Define to 1 when kernel is higher half])
  AH_TEMPLATE([INITRD_LOAD_ADDRESS], [Define contains initrd load address])
  AH_TEMPLATE([REMOTE_DEBUG], [Define to 1 to enable remote debugging])
  AH_TEMPLATE([FDT_BINARY], [Define to path to binary])
  AH_TEMPLATE([FDT_EMBED], [Define to 1 if you want to embed binary])
  # Output related define templates
  AH_TEMPLATE([OUTPUT_ENABLE], [Define to 1 to enable kernel print])
  AH_TEMPLATE([PRINT_MM_PHYS], [Define to 1 to enable output of physical memory manager])
  AH_TEMPLATE([PRINT_MM_VIRT], [Define to 1 to enable output of virtual memory manager])
  AH_TEMPLATE([PRINT_MM_HEAP], [Define to 1 to enable output of kernel heap])
  AH_TEMPLATE([PRINT_MAILBOX], [Define to 1 to enable output of mailbox])
  AH_TEMPLATE([PRINT_TIMER], [Define to 1 to enable output of timer])
  AH_TEMPLATE([PRINT_INITRD], [Define to 1 to enable output of initrd])
  AH_TEMPLATE([PRINT_EVENT], [Define to 1 to enable output of event])
  AH_TEMPLATE([PRINT_INTERRUPT], [Define to 1 to enable output of interrupt methods])
  AH_TEMPLATE([PRINT_PROCESS], [Define to 1 to enable output of process methods])
  AH_TEMPLATE([PRINT_EXCEPTION], [Define to 1 to enable output of exception handlers])
  AH_TEMPLATE([PRINT_ELF], [Define to 1 to enable output of elf routines])
  AH_TEMPLATE([PRINT_PLATFORM], [Define to 1 to enable output of platform initialization])
  AH_TEMPLATE([PRINT_SYSCALL], [Define to 1 to enable output of syscall initialization])
  AH_TEMPLATE([PRINT_SERIAL], [Define to 1 to enable output of serial handling])

  # Test for debugging enabled
  AS_IF([test "x$enable_remote_debug" == "xyes"], [
    AC_DEFINE([REMOTE_DEBUG], [1])
  ])

  # Test for general output enable
  AS_IF([test "x$enable_output" == "xyes"], [
    AC_DEFINE([OUTPUT_ENABLE], [1])
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

  # Test for timer output
  AS_IF([test "x$enable_output_timer" == "xyes"], [
    AC_DEFINE([PRINT_TIMER], [1])
  ])

  # Test for initrd output
  AS_IF([test "x$enable_output_initrd" == "xyes"], [
    AC_DEFINE([PRINT_INITRD], [1])
  ])

  # Test for event output
  AS_IF([test "x$enable_output_event" == "xyes"], [
    AC_DEFINE([PRINT_EVENT], [1])
  ])

  # Test for interrupt output
  AS_IF([test "x$enable_output_interrupt" == "xyes"], [
    AC_DEFINE([PRINT_INTERRUPT], [1])
  ])

  # Test for process output
  AS_IF([test "x$enable_output_process" == "xyes"], [
    AC_DEFINE([PRINT_PROCESS], [1])
  ])

  # Test for exception output
  AS_IF([test "x$enable_output_exception" == "xyes"], [
    AC_DEFINE([PRINT_EXCEPTION], [1])
  ])

  # Test for elf output
  AS_IF([test "x$enable_output_elf" == "xyes"], [
    AC_DEFINE([PRINT_ELF], [1])
  ])

  # Test for platform output
  AS_IF([test "x$enable_output_platform" == "xyes"], [
    AC_DEFINE([PRINT_PLATFORM],[1])
  ])

  # Test for syscall output
  AS_IF([test "x$enable_output_syscall" == "xyes"], [
    AC_DEFINE([PRINT_SYSCALL],[1])
  ])

  # Test for serial output
  AS_IF([test "x$enable_output_serial" == "xyes"], [
    AC_DEFINE([PRINT_SERIAL],[1])
  ])

  case "${host_cpu}" in
  arm)
    arch_subdir=arm
    host_bfd=elf32-littlearm
    output_img=kernel.img
    output_sym=kernel.sym
    output_map=kernel.map
    output_img_qemu=kernel_qemu.img
    output_sym_qemu=kernel_qemu.sym
    output_map_qemu=kernel_qemu.map
    executable_format=32
    AC_DEFINE([ELF32], [1])
    CFLAGS="${CFLAGS} -marm"

    case "${DEVICE}" in
    rpi2_b_rev1)
      CFLAGS="${CFLAGS} -march=armv7-a -mtune=cortex-a7 -mfpu=neon-vfpv4 -mfloat-abi=hard"
      subarch_subdir=v7
      platform_subdir=rpi
      output_img=kernel7.img
      output_sym=kernel7.sym
      output_img_qemu=kernel7_qemu.img
      output_sym_qemu=kernel7_qemu.sym
      AC_DEFINE([ELF32])
      AC_DEFINE([BCM2836], [1], [Define to 1 for BCM2836 chip])
      AC_DEFINE([ARCH_ARM_V7], [1], [Define to 1 for ARMv7 targets])
      AC_DEFINE([ARCH_ARM_CORTEX_A7], [1], [Define to 1 for ARM Cortex-A7 targets])
      AC_DEFINE([IS_HIGHER_HALF], [1])
      AC_DEFINE([INITRD_LOAD_ADDRESS], [0x800000])
      AC_DEFINE_UNQUOTED([FDT_BINARY], ["$($BOLTHUR_READLINK -f ${srcdir})/dts/rpi/bcm2836-rpi-2b.dtb"])
      ;;
    rpi_zero_w)
      CFLAGS="${CFLAGS} -march=armv6zk -mtune=arm1176jzf-s -mfpu=vfpv2 -mfloat-abi=hard"
      subarch_subdir=v6
      platform_subdir=rpi
      AC_DEFINE([BCM2835], [1], [Define to 1 for BCM2835])
      AC_DEFINE([ARCH_ARM_V6], [1], [Define to 1 for ARMv6 targets])
      AC_DEFINE([ARCH_ARM_ARM1176JZF_S], [1], [Define to 1 for ARM ARM1176JZF-S targets])
      AC_DEFINE([IS_HIGHER_HALF], [1])
      AC_DEFINE([INITRD_LOAD_ADDRESS], [0x800000])
      AC_DEFINE_UNQUOTED([FDT_BINARY], ["$($BOLTHUR_READLINK -f ${srcdir})/dts/rpi/bcm2835-rpi-zero-w.dtb"])
      ;;
    rpi3_b)
      CFLAGS="${CFLAGS} -march=armv8-a -mtune=cortex-a53 -mfpu=neon-vfpv4 -mfloat-abi=hard"
      subarch_subdir=v8
      platform_subdir=rpi
      output_img=kernel8.img
      output_sym=kernel8.sym
      output_img_qemu=kernel8_qemu.img
      output_sym_qemu=kernel8_qemu.sym
      AC_DEFINE([BCM2837], [1], [Define to 1 for BCM2837 chip])
      AC_DEFINE([ARCH_ARM_V8], [1], [Define to 1 for ARMv8 targets])
      AC_DEFINE([ARCH_ARM_CORTEX_A53], [1], [Define to 1 for ARM Cortex-A53 targets])
      AC_DEFINE([IS_HIGHER_HALF], [1])
      AC_DEFINE([INITRD_LOAD_ADDRESS], [0x800000])
      ;;
    *)
      AC_MSG_ERROR([unsupported host platform])
      ;;
    esac
    ;;
  aarch64)
    arch_subdir=arm
    host_bfd=elf64-littleaarch64
    executable_format=64
    AC_DEFINE([ELF64], [1])

    case "${DEVICE}" in
    rpi3_b)
      CFLAGS="${CFLAGS} -march=armv8-a -mtune=cortex-a53"
      subarch_subdir=v8
      platform_subdir=rpi
      output_img=kernel8.img
      output_sym=kernel8.sym
      output_img_qemu=kernel8_qemu.img
      output_sym_qemu=kernel8_qemu.sym
      AC_DEFINE([BCM2837], [1], [Define to 1 for BCM2837 chip])
      AC_DEFINE([ARCH_ARM_V8], [1], [Define to 1 for ARMv8 targets])
      AC_DEFINE([ARCH_ARM_CORTEX_A53], [1], [Define to 1 for ARM Cortex-A53 targets])
      AC_DEFINE([IS_HIGHER_HALF], [1])
      AC_DEFINE([INITRD_LOAD_ADDRESS], [0x800000])
      ;;
    *)
      AC_MSG_ERROR([unsupported host platform])
      ;;
    esac
    ;;
  *)
    AC_MSG_ERROR([unsupported host CPU])
    ;;
  esac

  # copy flags for binary creation
  copy_flags="-I ${host_bfd} -O ${host_bfd}"

  AC_DEFINE_UNQUOTED([ARCH], [${arch_subdir}], [bolthur/kernel target architecture])
  AC_DEFINE_UNQUOTED([SUBARCH], [${subarch_subdir}], [bolthur/kernel target subarchitecture])
  AC_DEFINE_UNQUOTED([PLATFORM], [${platform_subdir}], [bolthur/kernel target platform])
  AC_DEFINE_UNQUOTED(
    [PACKAGE_ARCHITECTURE],
    ["$arch_subdir$subarch_subdir"],
    [Define to the target architecture for this package]
  )
  AC_SUBST(arch_subdir)
  AC_SUBST(subarch_subdir)
  AC_SUBST(platform_subdir)
  AC_SUBST(output_img)
  AC_SUBST(output_sym)
  AC_SUBST(output_map)
  AC_SUBST(output_img_qemu)
  AC_SUBST(output_sym_qemu)
  AC_SUBST(output_map_qemu)
  AC_SUBST(host_bfd)
  AC_SUBST(copy_flags)
  AC_SUBST(executable_format)
])
