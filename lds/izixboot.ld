/* lds/izixboot.ld */

/* Our entry point is _start. */
ENTRY(_start)

SECTIONS
{
    /**
     * Our ELF will start at 0x8000 and our .text section at a 4k offset.
     */
    . = 0x8000;

    /**
     * Lets start our kernels entry point at 0x9000,
     * so that it can be loaded by our bootloader while in real mode.
     */
    . = 0x9000;
    .text BLOCK(4K) : ALIGN(4K)
    {
        *(.text)
    }

    /**
     * Our initialization and finalization sections.
     */
    .init BLOCK(4K) : ALIGN(4K)
    {
        *(.init)
    }
    .fini BLOCK(4K) : ALIGN(4K)
    {
        *(.fini)
    }

    /* Read-only data. */
    .rodata BLOCK(4K) : ALIGN(4K)
    {
        *(.rodata)
    }

    /* Read-write data (initialized) */
    .data BLOCK(4K) : ALIGN(4K)
    {
        *(.data)
        *(COMMON)
        /* Kill the BSS section, since our bootloader doesn't initialize it. */
        *(.bss)
    }
}

/* vim: set ts=4 sw=4 et syn=ld: */
