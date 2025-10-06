OUTPUT_FORMAT(elf64-x86-64)
OUTPUT_ARCH(i386:x86-64)
ENTRY(kmain)

/* Program headers (segments) for proper memory protection */
PHDRS {
  text   PT_LOAD FLAGS(5);  /* R-X */
  rodata PT_LOAD FLAGS(4);  /* R-- */
  data   PT_LOAD FLAGS(6);  /* RW- */
}

SECTIONS {
  /* Higher-half kernel base (top 2GiB of virtual address space) */
  . = 0xffffffff80000000;

  /* Keep Limine request blocks together at the start */
  .limine_reqs : {
    KEEP(*(.limine_reqs_start))
    KEEP(*(.limine_reqs))
    KEEP(*(.limine_reqs.*))
    KEEP(*(.limine_reqs_end))
    /* Also accept alternate section names for compatibility */
    KEEP(*(.limine_requests_start))
    KEEP(*(.limine_requests))
    KEEP(*(.limine_requests.*))
    KEEP(*(.limine_requests_end))
  } :text

  /* Align to page boundary (4KiB) */
  . = ALIGN(CONSTANT(MAXPAGESIZE));

  /* Executable code */
  .text : {
    *(.text.boot)  /* Boot code first */
    *(.text .text.*)
  } :text

  /* Align to next page */
  . = ALIGN(CONSTANT(MAXPAGESIZE));

  /* Read-only data */
  .rodata : {
    *(.rodata .rodata.*)
    *(.eh_frame)
    *(.note.gnu.build-id)
  } :rodata

  /* Align to next page */
  . = ALIGN(CONSTANT(MAXPAGESIZE));

  /* Initialized data */
  .data : {
    *(.data .data.*)
  } :data

  /* Uninitialized data (BSS) */
  .bss : {
    *(COMMON)
    *(.bss .bss.*)
  } :data

  /* Discard unnecessary sections */
  /DISCARD/ : {
    *(.comment)
    *(.note)
    *(.note.*)
  }
}
