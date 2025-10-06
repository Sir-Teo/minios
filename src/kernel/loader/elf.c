#include "elf.h"
#include "../kprintf.h"
#include "../mm/pmm.h"
#include "../mm/kmalloc.h"
#include "../../arch/x86_64/mm/vmm.h"
#include <stddef.h>

// Freestanding C library functions (from support.c)
extern void *memcpy(void *restrict dest, const void *restrict src, size_t n);
extern void *memset(void *dest, int c, size_t n);

// Limine HHDM request
#include "../limine.h"
extern volatile struct limine_hhdm_request hhdm_request;

// Error codes
#define ELF_SUCCESS           0
#define ELF_ERR_INVALID_MAGIC -1
#define ELF_ERR_NOT_64BIT     -2
#define ELF_ERR_NOT_LITTLE_ENDIAN -3
#define ELF_ERR_INVALID_VERSION -4
#define ELF_ERR_NOT_EXECUTABLE -5
#define ELF_ERR_NOT_X86_64    -6
#define ELF_ERR_NO_PROGRAM_HEADERS -7
#define ELF_ERR_INVALID_SIZE  -8
#define ELF_ERR_ALLOC_FAILED  -9
#define ELF_ERR_MAP_FAILED    -10

static int last_error = ELF_SUCCESS;

/**
 * Initialize the ELF loader subsystem
 */
void elf_init(void) {
    kprintf("[ELF] Initializing ELF loader subsystem\n");
    last_error = ELF_SUCCESS;
}

/**
 * Validate an ELF file
 *
 * @param elf_data Pointer to ELF file data
 * @param size Size of ELF file in bytes
 * @return true if valid, false otherwise
 */
bool elf_validate(const void *elf_data, size_t size) {
    if (!elf_data || size < sizeof(elf64_ehdr_t)) {
        last_error = ELF_ERR_INVALID_SIZE;
        return false;
    }

    const elf64_ehdr_t *header = (const elf64_ehdr_t *)elf_data;

    // Check ELF magic number
    if (header->e_ident[EI_MAG0] != 0x7F ||
        header->e_ident[EI_MAG1] != 'E' ||
        header->e_ident[EI_MAG2] != 'L' ||
        header->e_ident[EI_MAG3] != 'F') {
        last_error = ELF_ERR_INVALID_MAGIC;
        return false;
    }

    // Check for 64-bit ELF
    if (header->e_ident[EI_CLASS] != ELF_CLASS_64) {
        last_error = ELF_ERR_NOT_64BIT;
        return false;
    }

    // Check for little-endian
    if (header->e_ident[EI_DATA] != ELF_DATA_2LSB) {
        last_error = ELF_ERR_NOT_LITTLE_ENDIAN;
        return false;
    }

    // Check version
    if (header->e_ident[EI_VERSION] != ELF_VERSION_CURRENT) {
        last_error = ELF_ERR_INVALID_VERSION;
        return false;
    }

    // Check file type (must be executable)
    if (header->e_type != ET_EXEC && header->e_type != ET_DYN) {
        last_error = ELF_ERR_NOT_EXECUTABLE;
        return false;
    }

    // Check machine type (must be x86_64)
    if (header->e_machine != EM_X86_64) {
        last_error = ELF_ERR_NOT_X86_64;
        return false;
    }

    // Check for program headers
    if (header->e_phnum == 0 || header->e_phoff == 0) {
        last_error = ELF_ERR_NO_PROGRAM_HEADERS;
        return false;
    }

    last_error = ELF_SUCCESS;
    return true;
}

/**
 * Load an ELF file into memory
 *
 * This function:
 * 1. Validates the ELF file
 * 2. Allocates physical memory for loadable segments
 * 3. Copies segment data from the ELF file
 * 4. Zeros BSS sections
 * 5. Returns the entry point address
 *
 * @param elf_data Pointer to ELF file data
 * @param size Size of ELF file in bytes
 * @param entry_point Output parameter for entry point address
 * @return Address space with loaded program, or NULL on failure
 */
void *elf_load(const void *elf_data, size_t size, uint64_t *entry_point) {
    if (!elf_validate(elf_data, size)) {
        return NULL;
    }

    const elf64_ehdr_t *header = (const elf64_ehdr_t *)elf_data;
    const elf64_phdr_t *phdrs = (const elf64_phdr_t *)((uint8_t *)elf_data + header->e_phoff);

    // Create a new address space for the loaded program
    address_space_t *address_space = vmm_create_address_space();
    if (!address_space) {
        last_error = ELF_ERR_ALLOC_FAILED;
        return NULL;
    }

    kprintf("[ELF] Loading ELF binary with %d program headers\n", header->e_phnum);

    // Load all LOAD segments
    for (uint16_t i = 0; i < header->e_phnum; i++) {
        const elf64_phdr_t *phdr = &phdrs[i];

        // Skip non-loadable segments
        if (phdr->p_type != PT_LOAD) {
            continue;
        }

        kprintf("[ELF] Loading segment %d: vaddr=0x%lx size=0x%lx flags=%c%c%c\n",
                i, phdr->p_vaddr, phdr->p_memsz,
                (phdr->p_flags & PF_R) ? 'R' : '-',
                (phdr->p_flags & PF_W) ? 'W' : '-',
                (phdr->p_flags & PF_X) ? 'X' : '-');

        // Calculate aligned addresses and sizes
        uint64_t vaddr_aligned = phdr->p_vaddr & ~0xFFF;
        uint64_t vaddr_offset = phdr->p_vaddr & 0xFFF;
        uint64_t memsz_aligned = (phdr->p_memsz + vaddr_offset + 0xFFF) & ~0xFFF;
        uint64_t num_pages = memsz_aligned / 0x1000;

        // Determine page flags
        uint64_t flags = VMM_PRESENT | VMM_USER;  // User-accessible
        if (phdr->p_flags & PF_W) {
            flags |= VMM_WRITABLE;
        }
        if (!(phdr->p_flags & PF_X)) {
            flags |= VMM_NO_EXECUTE;  // No-execute for non-executable segments
        }

        // Allocate and map pages for this segment
        for (uint64_t page = 0; page < num_pages; page++) {
            uint64_t vaddr = vaddr_aligned + (page * 0x1000);
            uint64_t phys = pmm_alloc();

            if (phys == 0) {
                kprintf("[ELF] Failed to allocate physical page for segment %d\n", i);
                // TODO: Clean up already allocated pages
                vmm_destroy_address_space(address_space);
                last_error = ELF_ERR_ALLOC_FAILED;
                return NULL;
            }

            // Map the page
            if (!vmm_map_page(address_space, vaddr, phys, flags)) {
                kprintf("[ELF] Failed to map page at vaddr=0x%lx\n", vaddr);
                pmm_free(phys);
                // TODO: Clean up already allocated pages
                vmm_destroy_address_space(address_space);
                last_error = ELF_ERR_MAP_FAILED;
                return NULL;
            }
        }

        // Copy segment data from ELF file to allocated memory
        // We need to temporarily map the pages to kernel space to copy data
        const uint8_t *src = (const uint8_t *)elf_data + phdr->p_offset;
        uint64_t bytes_to_copy = phdr->p_filesz;

        for (uint64_t offset = 0; offset < bytes_to_copy; ) {
            uint64_t vaddr = phdr->p_vaddr + offset;
            uint64_t page_vaddr = vaddr & ~0xFFF;
            uint64_t page_offset = vaddr & 0xFFF;
            uint64_t copy_size = 0x1000 - page_offset;
            if (copy_size > bytes_to_copy - offset) {
                copy_size = bytes_to_copy - offset;
            }

            // Get the physical address for this page
            uint64_t phys = vmm_get_physical(address_space, page_vaddr);
            if (phys == 0) {
                kprintf("[ELF] Failed to get physical address for vaddr=0x%lx\n", page_vaddr);
                vmm_destroy_address_space(address_space);
                last_error = ELF_ERR_MAP_FAILED;
                return NULL;
            }

            // Copy data using HHDM (Higher Half Direct Map)
            uint8_t *dest = (uint8_t *)(phys + hhdm_request.response->offset + page_offset);
            memcpy(dest, src + offset, copy_size);

            offset += copy_size;
        }

        // Zero out BSS section (p_memsz > p_filesz)
        if (phdr->p_memsz > phdr->p_filesz) {
            uint64_t bss_start = phdr->p_vaddr + phdr->p_filesz;
            uint64_t bss_size = phdr->p_memsz - phdr->p_filesz;

            kprintf("[ELF] Zeroing BSS section: vaddr=0x%lx size=0x%lx\n", bss_start, bss_size);

            for (uint64_t offset = 0; offset < bss_size; ) {
                uint64_t vaddr = bss_start + offset;
                uint64_t page_vaddr = vaddr & ~0xFFF;
                uint64_t page_offset = vaddr & 0xFFF;
                uint64_t zero_size = 0x1000 - page_offset;
                if (zero_size > bss_size - offset) {
                    zero_size = bss_size - offset;
                }

                // Get the physical address for this page
                uint64_t phys = vmm_get_physical(address_space, page_vaddr);
                if (phys == 0) {
                    kprintf("[ELF] Failed to get physical address for BSS vaddr=0x%lx\n", page_vaddr);
                    vmm_destroy_address_space(address_space);
                    last_error = ELF_ERR_MAP_FAILED;
                    return NULL;
                }

                // Zero BSS using HHDM
                uint8_t *dest = (uint8_t *)(phys + hhdm_request.response->offset + page_offset);
                memset(dest, 0, zero_size);

                offset += zero_size;
            }
        }
    }

    // Set entry point
    if (entry_point) {
        *entry_point = header->e_entry;
    }

    kprintf("[ELF] Successfully loaded ELF binary, entry point: 0x%lx\n", header->e_entry);
    last_error = ELF_SUCCESS;
    return address_space;
}

/**
 * Get error string for last ELF error
 *
 * @param error Error code (or 0 to use last_error)
 * @return Error string
 */
const char *elf_strerror(int error) {
    if (error == 0) {
        error = last_error;
    }

    switch (error) {
        case ELF_SUCCESS:
            return "Success";
        case ELF_ERR_INVALID_MAGIC:
            return "Invalid ELF magic number";
        case ELF_ERR_NOT_64BIT:
            return "Not a 64-bit ELF file";
        case ELF_ERR_NOT_LITTLE_ENDIAN:
            return "Not a little-endian ELF file";
        case ELF_ERR_INVALID_VERSION:
            return "Invalid ELF version";
        case ELF_ERR_NOT_EXECUTABLE:
            return "Not an executable ELF file";
        case ELF_ERR_NOT_X86_64:
            return "Not an x86_64 ELF file";
        case ELF_ERR_NO_PROGRAM_HEADERS:
            return "No program headers found";
        case ELF_ERR_INVALID_SIZE:
            return "Invalid ELF file size";
        case ELF_ERR_ALLOC_FAILED:
            return "Memory allocation failed";
        case ELF_ERR_MAP_FAILED:
            return "Page mapping failed";
        default:
            return "Unknown error";
    }
}
