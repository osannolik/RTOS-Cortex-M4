/* This will work with STM32 type of microcontrollers.    *
 * The sizes of RAM and flash are specified smaller than  *
 * what most of the STM32 provide to ensure that the demo *
 * program will run on ANY STM32.                         */
MEMORY
{
    RAM (rwx) : ORIGIN = 0x20000000, LENGTH = 128K
    FLASH (rx)  : ORIGIN = 0x08000000, LENGTH = 1024K
}

SECTIONS
{
    .  = 0x0;         /* From 0x00000000 */

    .text :
    {
        _stext = .;
        *(vectors)    /* Vector table */
        *(.text)      /* Program code */
        *(.text.*)      /*  */
    } >FLASH

    .rodata :
    {
        *(.rodata)    /* Read only data */
        *(.rodata.*)    /* Read only data */
    } >FLASH

    _etext = .;

    .  = 0x20000000;  /* From 0x20000000 */
    
    .data :
    {
        _sdata = .;
        *(.data)	   /* Data memory */
        *(.data*)      /* .data* sections */
    } >RAM AT > FLASH
    _edata = .;
    
    .bss :
    {
        _sbss = .;
        *(.bss)       /* Zero-filled run time allocate data memory */
    } >RAM AT > FLASH
    _ebss = .;

    .stack_seg :
    {
        _stack_bottom = .;
        KEEP(*(.stack*))
    } > RAM

    _stack_top = .;
}
