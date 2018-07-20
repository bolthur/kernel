
OUTPUT_FORMAT( elf32-i386 )
OUTPUT_ARCH( i386 )

ENTRY( start )

GROUP( ../../arch/x86/ia32/libia32.a ../../arch/x86/libx86.a ../../libkernel.a libpc.a ../../../libc/libc.a )
INPUT( libgcc.a )

SECTIONS {
  . = 1M;
  __kernel_start = .;

  .text BLOCK( 4K ): ALIGN( 4K ) {
    *(.multiboot)
    *(.text)
  }

  .rodata BLOCK( 4K ): ALIGN( 4K ) {
    *(.rodata)
  }

  .data BLOCK( 4K ) : ALIGN( 4K ) {
    *(.data)
  }

  .bss BLOCK( 4K ) : ALIGN( 4K ) {
    __bss_start = .;
    *(COMMON)
    *(.bss)
    __bss_end = .;
  }

  __kernel_end = .;
}
