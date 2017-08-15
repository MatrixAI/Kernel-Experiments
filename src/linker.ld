/* The boot laoder will look at this image and start execution at the symbol designated as the entry point */
ENTRY(_start)

/* Tell where the various sections of the object files will be put */
SECTIONS
{
  . = 1M; /* start putting sections are 1 MiB, so this is a conventional place for kernels to be loaded by the bootloader */

  /* first put the multiboot header as it is required be very early */
  /* then the text section */
  .text BLOCK(4K) : ALIGN(4K)
  {
    *(.multiboot)
    *(.text)
  }

  .rodata BLOCK(4K) : ALIGN(4K)
  {
    *(.rodata)
  }

  .data BLOCK(4K) : ALIGN(4K)
  {
    *(.data)
  }

  /* here is our stack, is the order of this significant? */
  .bss BLOCK(4K) : ALIGN(4K)
  {
    *(COMMON)
    *(.bss)
  }

}
