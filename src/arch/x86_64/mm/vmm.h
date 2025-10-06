#ifndef ARCH_X86_64_VMM_H
#define ARCH_X86_64_VMM_H

#include <stdint.h>
#include <stdbool.h>

// Page table entry flags (x86_64 architecture)
#define VMM_PRESENT        (1UL << 0)   // Page is present in memory
#define VMM_WRITABLE       (1UL << 1)   // Page is writable
#define VMM_USER           (1UL << 2)   // Page is accessible from user mode
#define VMM_WRITETHROUGH   (1UL << 3)   // Write-through caching
#define VMM_CACHE_DISABLE  (1UL << 4)   // Disable caching
#define VMM_ACCESSED       (1UL << 5)   // Page has been accessed
#define VMM_DIRTY          (1UL << 6)   // Page has been written to
#define VMM_HUGE           (1UL << 7)   // Huge page (2MB or 1GB)
#define VMM_GLOBAL         (1UL << 8)   // Global page (not flushed on CR3 switch)
#define VMM_NO_EXECUTE     (1UL << 63)  // No execute (NX bit)

// Page sizes
#define PAGE_SIZE          0x1000       // 4 KiB
#define PAGE_SIZE_2M       0x200000     // 2 MiB
#define PAGE_SIZE_1G       0x40000000   // 1 GiB

// Page table indices extraction macros
#define PML4_INDEX(addr) (((addr) >> 39) & 0x1FF)
#define PDPT_INDEX(addr) (((addr) >> 30) & 0x1FF)
#define PD_INDEX(addr)   (((addr) >> 21) & 0x1FF)
#define PT_INDEX(addr)   (((addr) >> 12) & 0x1FF)

// Page alignment macros
#define PAGE_ALIGN_DOWN(addr) ((addr) & ~(PAGE_SIZE - 1))
#define PAGE_ALIGN_UP(addr)   (((addr) + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1))
#define IS_PAGE_ALIGNED(addr) (((addr) & (PAGE_SIZE - 1)) == 0)

// Get physical address from page table entry
#define PTE_GET_ADDR(pte) ((pte) & 0x000FFFFFFFFFF000UL)

// Address space structure (per-process page tables)
typedef struct address_space {
    uint64_t *pml4_virt;      // Virtual address of PML4
    uint64_t pml4_phys;       // Physical address of PML4
    uint64_t ref_count;       // Reference count for shared spaces
} address_space_t;

// Initialize VMM (sets up kernel address space)
void vmm_init(void);

// Create a new address space (for user processes)
address_space_t *vmm_create_address_space(void);

// Destroy an address space and free its resources
void vmm_destroy_address_space(address_space_t *as);

// Clone an address space (for fork)
address_space_t *vmm_clone_address_space(address_space_t *src);

// Map a virtual page to a physical page
bool vmm_map_page(address_space_t *as, uint64_t virt, uint64_t phys, uint64_t flags);

// Unmap a virtual page
bool vmm_unmap_page(address_space_t *as, uint64_t virt);

// Get physical address from virtual address
uint64_t vmm_get_physical(address_space_t *as, uint64_t virt);

// Check if a virtual address is mapped
bool vmm_is_mapped(address_space_t *as, uint64_t virt);

// Switch to an address space (load CR3)
void vmm_switch_address_space(address_space_t *as);

// Get current address space
address_space_t *vmm_get_current_space(void);

// Get kernel address space
address_space_t *vmm_get_kernel_space(void);

// Flush TLB for a specific virtual address
void vmm_invlpg(uint64_t virt);

// Flush entire TLB
void vmm_flush_tlb(void);

#endif
