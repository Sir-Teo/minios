#ifndef ELF_H
#define ELF_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

// ELF magic number
#define ELF_MAGIC 0x464C457F  // "\x7FELF" in little endian

// ELF classes
#define ELF_CLASS_NONE 0
#define ELF_CLASS_32   1
#define ELF_CLASS_64   2

// ELF data encodings
#define ELF_DATA_NONE 0
#define ELF_DATA_2LSB 1  // Little endian
#define ELF_DATA_2MSB 2  // Big endian

// ELF versions
#define ELF_VERSION_CURRENT 1

// ELF file types
#define ET_NONE   0  // No file type
#define ET_REL    1  // Relocatable file
#define ET_EXEC   2  // Executable file
#define ET_DYN    3  // Shared object file
#define ET_CORE   4  // Core file

// ELF machine types
#define EM_NONE   0   // No machine
#define EM_X86_64 62  // AMD x86-64

// Program header types
#define PT_NULL    0  // Unused entry
#define PT_LOAD    1  // Loadable segment
#define PT_DYNAMIC 2  // Dynamic linking information
#define PT_INTERP  3  // Interpreter path
#define PT_NOTE    4  // Auxiliary information
#define PT_SHLIB   5  // Reserved
#define PT_PHDR    6  // Program header table
#define PT_TLS     7  // Thread-local storage

// Program header flags
#define PF_X 0x1  // Execute
#define PF_W 0x2  // Write
#define PF_R 0x4  // Read

// Section header types
#define SHT_NULL     0   // Unused entry
#define SHT_PROGBITS 1   // Program data
#define SHT_SYMTAB   2   // Symbol table
#define SHT_STRTAB   3   // String table
#define SHT_RELA     4   // Relocation entries with addends
#define SHT_HASH     5   // Symbol hash table
#define SHT_DYNAMIC  6   // Dynamic linking information
#define SHT_NOTE     7   // Notes
#define SHT_NOBITS   8   // BSS (uninitialized data)
#define SHT_REL      9   // Relocation entries without addends
#define SHT_SHLIB    10  // Reserved
#define SHT_DYNSYM   11  // Dynamic linker symbol table

// Section header flags
#define SHF_WRITE     0x1  // Writable
#define SHF_ALLOC     0x2  // Occupies memory during execution
#define SHF_EXECINSTR 0x4  // Executable

// ELF identification indices
#define EI_MAG0       0  // Magic number byte 0
#define EI_MAG1       1  // Magic number byte 1
#define EI_MAG2       2  // Magic number byte 2
#define EI_MAG3       3  // Magic number byte 3
#define EI_CLASS      4  // File class
#define EI_DATA       5  // Data encoding
#define EI_VERSION    6  // File version
#define EI_OSABI      7  // OS/ABI identification
#define EI_ABIVERSION 8  // ABI version
#define EI_PAD        9  // Start of padding bytes
#define EI_NIDENT     16 // Size of e_ident[]

// ELF64 header structure
typedef struct {
    uint8_t  e_ident[EI_NIDENT];  // Magic number and other info
    uint16_t e_type;               // Object file type
    uint16_t e_machine;            // Architecture
    uint32_t e_version;            // Object file version
    uint64_t e_entry;              // Entry point virtual address
    uint64_t e_phoff;              // Program header table file offset
    uint64_t e_shoff;              // Section header table file offset
    uint32_t e_flags;              // Processor-specific flags
    uint16_t e_ehsize;             // ELF header size in bytes
    uint16_t e_phentsize;          // Program header table entry size
    uint16_t e_phnum;              // Program header table entry count
    uint16_t e_shentsize;          // Section header table entry size
    uint16_t e_shnum;              // Section header table entry count
    uint16_t e_shstrndx;           // Section header string table index
} __attribute__((packed)) elf64_ehdr_t;

// ELF64 program header structure
typedef struct {
    uint32_t p_type;    // Segment type
    uint32_t p_flags;   // Segment flags
    uint64_t p_offset;  // Segment file offset
    uint64_t p_vaddr;   // Segment virtual address
    uint64_t p_paddr;   // Segment physical address
    uint64_t p_filesz;  // Segment size in file
    uint64_t p_memsz;   // Segment size in memory
    uint64_t p_align;   // Segment alignment
} __attribute__((packed)) elf64_phdr_t;

// ELF64 section header structure
typedef struct {
    uint32_t sh_name;       // Section name (string table index)
    uint32_t sh_type;       // Section type
    uint64_t sh_flags;      // Section flags
    uint64_t sh_addr;       // Section virtual address at execution
    uint64_t sh_offset;     // Section file offset
    uint64_t sh_size;       // Section size in bytes
    uint32_t sh_link;       // Link to another section
    uint32_t sh_info;       // Additional section information
    uint64_t sh_addralign;  // Section alignment
    uint64_t sh_entsize;    // Entry size if section holds table
} __attribute__((packed)) elf64_shdr_t;

// Function declarations
void elf_init(void);
bool elf_validate(const void *elf_data, size_t size);
void *elf_load(const void *elf_data, size_t size, uint64_t *entry_point);
const char *elf_strerror(int error);

#endif // ELF_H
