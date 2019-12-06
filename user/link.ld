
OUTPUT_FORMAT( elf32-littlearm )
OUTPUT_ARCH( arm )

ENTRY( _start )

SECTIONS {
  . = 0x200000;

  .text : {
    *( .text )
  }

  .data ALIGN( 0x1000 ) : {
    *( .data )
  }

  .rodata ALIGN( 0x1000 ) : {
    *( .rodata )
  }

  .bss ALIGN( 0x1000 ) : {
    *( .bss )
  }
}
