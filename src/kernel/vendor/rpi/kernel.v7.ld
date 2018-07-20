OUTPUT_FORMAT( elf32-littlearm )
OUTPUT_ARCH( arm )

ENTRY( start )

GROUP( ../../arch/arm/v7/libv7.a ../../arch/arm/libarm.a ../../libkernel.a librpi.a ../../../libc/libc.a )
INPUT( libgcc.a )

SECTIONS {
  . = 0x8000;
  __kernel_start = .;

  .text BLOCK( 4K ): ALIGN( 4K ) {
    KEEP( *( .text.boot ) )
    *( .text )
  }

  .rodata BLOCK( 4K ): ALIGN( 4K ) {
    *(.rodata)
  }

  .data BLOCK( 4K ): ALIGN( 4K ) {
    *(.data)
  }

  .bss BLOCK( 4K ): ALIGN( 4K ) {
    __bss_start = .;
    *(COMMON)
    *(.bss)
    __bss_end = .;
  }
  __kernel_end = .;
}
