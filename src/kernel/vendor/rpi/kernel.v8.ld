OUTPUT_FORMAT( elf64-littleaarch64 )
OUTPUT_ARCH( arm )

ENTRY( start )

GROUP( ../../arch/arm/v8/libv8.a ../../arch/arm/libarm.a ../../libkernel.a ../libvendor.a librpi.a ../../../libc/libc.a )
INPUT( libgcc.a )

SECTIONS {
  . = 0x80000;
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

  .bugtable BLOCK( 4K ): ALIGN( 4K ) {
    __bug_start = . ;
    *(.bugtable)
    __bug_end = . ;
  }
  __kernel_end = .;

  /*
  /DISCARD/ : {
    *(.comment*)
    *(.ARM*)
    *(.note*)
  }*/
}
