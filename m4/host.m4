
AC_DEFUN([MIST_SET_HOST], [
  AH_TEMPLATE([ELF32], [Define to 1 for 32 bit ELF targets.])
  AH_TEMPLATE([ELF64], [Define to 1 for 64 bit ELF targets.])
  AC_DEFINE([IS_KERNEL], [1], [Define set for libc to compile differently.])

  case "${host_cpu}" in
  i*86)
    arch_subdir=x86
    subarch_subdir=ia32
    host_bfd=elf32-i386
    copy_flags="-I ${host_bfd} -O ${host_bfd}"
    CFLAGS="${CFLAGS} -target i386-pc-elf"
    AC_DEFINE([ELF32])
    AC_DEFINE([ARCH_X86_IA32], [1], [Define to 1 for IA32 targets.])
    case "${DEVICE}" in
    pc)
      vendor_subdir=pc
      AC_DEFINE([VENDOR_PC], [1], [Define to 1 for PC vendor.])
      ;;
    *)
      AC_MSG_ERROR([unsupported host vendor])
      ;;
    esac
    ;;
  x86_64)
    arch_subdir=x86
    subarch_subdir=amd64
    host_bfd=elf64-x86-64
    copy_flags="-I ${host_bfd} -O ${host_bfd}"
    CFLAGS="${CFLAGS} -target x86_64-pc-elf -mcmodel=kernel -mno-red-zone"
    LDFLAGS="${LDFLAGS} -Wl,-z -Wl,max-page-size=0x1000"
    AC_DEFINE([ELF64])
    AC_DEFINE([ARCH_X86_AMD64], [1], [Define to 1 for AMD64 targets.])
    case "${DEVICE}" in
    pc)
      vendor_subdir=pc
      AC_DEFINE([VENDOR_PC], [1], [Define to 1 for PC vendor.])
      ;;
    *)
      AC_MSG_ERROR([unsupported host vendor])
      ;;
    esac
    ;;
  arm)
    arch_subdir=arm
    host_bfd=elf32-littlearm
    copy_flags="-I ${host_bfd} -O ${host_bfd}"
    AC_DEFINE([ARCH_ARM], [1], [Define to 1 for ARM targets.])
    case "${DEVICE}" in
    rpi2_1)
      #-target arm-none-eabi
      CFLAGS="${CFLAGS} -march=armv7-a -mtune=cortex-a7 -mfpu=neon-vfpv4 -mfloat-abi=hard"
      subarch_subdir=v7
      vendor_subdir=rpi
      AC_DEFINE([ELF32])
      AC_DEFINE([PLATFORM_RPI2], [1], [Define to 1 for raspberry pi 2 ( rev. 1 ) platform])
      AC_DEFINE([ARCH_ARM_V7], [1], [Define to 1 for ARMv7 targets.])
      AC_DEFINE([ARCH_ARM_CORTEX_A7], [1], [Define to 1 for ARM Cortex-A7 targets.])
      AC_DEFINE([PLATFORM_MAX_MEMORY_SIZE], [1024], [Define maximum platform memory size.])
      AC_DEFINE([VENDOR_RPI], [1], [Define to 1 for raspberry pi vendor.])
      ;;
    rpi2_2)
      #-target arm-none-eabi
      CFLAGS="${CFLAGS} -march=armv8-a -mtune=cortex-a53 -mfpu=vfp -mfloat-abi=hard"
      subarch_subdir=v8
      vendor_subdir=rpi
      AC_DEFINE([ELF64])
      AC_DEFINE([PLATFORM_RPI2], [1], [Define to 1 for raspberry pi 2 ( rev. 2 ) platform])
      AC_DEFINE([ARCH_ARM_V8], [1], [Define to 1 for ARMv7 targets.])
      AC_DEFINE([ARCH_ARM_CORTEX_A53], [1], [Define to 1 for ARM Cortex-A53 targets.])
      AC_DEFINE([PLATFORM_MAX_MEMORY_SIZE], [1024], [Define maximum platform memory size.])
      AC_DEFINE([VENDOR_RPI], [1], [Define to 1 for raspberry pi vendor.])
      ;;
    rpi3)
      #-target arm-none-eabi
      CFLAGS="${CFLAGS} -march=armv8-a -mtune=cortex-a53 -mfpu=vfp -mfloat-abi=hard"
      subarch_subdir=v8
      vendor_subdir=rpi
      AC_DEFINE([ELF64])
      AC_DEFINE([PLATFORM_RPI3], [1], [Define to 1 for raspberry pi 3 platform])
      AC_DEFINE([ARCH_ARM_V8], [1], [Define to 1 for ARMv8 targets.])
      AC_DEFINE([ARCH_ARM_CORTEX_A53], [1], [Define to 1 for ARM Cortex-A53 targets.])
      AC_DEFINE([PLATFORM_MAX_MEMORY_SIZE], [1024], [Define maximum platform memory size.])
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
  AC_DEFINE_UNQUOTED([ARCH], [${arch_subdir}], [mist-system/kernel target architecture.])
  AC_DEFINE_UNQUOTED([SUBARCH], [${subarch_subdir}], [mist-system/kernel target subarchitecture.])
  AC_DEFINE_UNQUOTED([VENDOR], [${vendor_subdir}], [mist-system/kernel target vendor.])
  AC_SUBST(arch_subdir)
  AC_SUBST(subarch_subdir)
  AC_SUBST(vendor_subdir)
  AC_SUBST(host_bfd)
  AC_SUBST(copy_flags)
])

AC_DEFUN([MIST_SET_FLAGS], [
  CFLAGS="${CFLAGS} -ffreestanding -Wall -Wextra -Werror -Wpedantic -nodefaultlibs"
  LDFLAGS="${LDFLAGS} -nostdlib -fno-exceptions"
])

AC_DEFUN([MIST_PROG_OBJCOPY], [
  AC_CHECK_TOOL([MIST_OBJCOPY], [objcopy])
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
