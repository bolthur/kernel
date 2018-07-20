
OUTPUT_FORMAT( elf64-x86-64 )
OUTPUT_ARCH( i386:x86-64 )

ENTRY( start )

GROUP( ../../arch/x86/amd64/libamd64.a ../../arch/x86/libx86.a ../../libkernel.a libpc.a ../../../libc/libc.a )
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
