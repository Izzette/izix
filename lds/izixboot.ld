/* lds/izixboot.ld */

/* Our entry point is _start. */
ENTRY(_start)

SECTIONS
{
    /**
     * Our ELF will start at 0x8000 and our .text section at a 4k offset.
     */
    . = 0x8000;

    __gnu_bssstart = ADDR(.bss);
    __gnu_bsslength = SIZEOF(.bss);
    __gnu_bssend = __gnu_bssstart + __gnu_bsslength;

    /**
     * Lets start our kernels entry point at 0x9000,
     * so that it can be loaded by our bootloader while in real mode.
     */
    . = 0x9000;
    .text BLOCK(4K) : ALIGN(4K)
    {
        *(.text)
        *(.init)
        *(.fini)
    }

    /* Data (initialized) */
    .data BLOCK(4K) : ALIGN(4K)
    {
        *(.data)
        *(.ctors)
        *(.dtors)
        *(.jcr)

        /**
         * There is no memory protection anyways,
         * so let's just shove it all in data.
         */
        *(.rodata)
        *(.rodata.*)
        *(.eh_frame)
    }

    /* Data (un-initialized) */
    .bss BLOCK(4K) : ALIGN(4K)
    {
        *(COMMON)
        *(.bss)
    }
}

/* vim: set ts=4 sw=4 et syn=ld: */
