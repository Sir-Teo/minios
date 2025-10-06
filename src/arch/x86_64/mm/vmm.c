#include "vmm.h"
#include "../../../kernel/mm/pmm.h"
#include "../../../kernel/mm/kmalloc.h"
#include "../../../kernel/limine.h"
#include <stddef.h>

// External declarations
extern void serial_write(const char *s);
extern void serial_write_hex(uint64_t value);
extern void *memset(void *d, int c, size_t n);
extern volatile struct limine_hhdm_request hhdm_request;

// Kernel address space
static address_space_t kernel_address_space;
static address_space_t *current_address_space = NULL;

// HHDM offset for converting physical to virtual addresses
static uint64_t hhdm_offset = 0;

// Convert physical address to virtual (using HHDM)
static inline void *phys_to_virt(uint64_t phys) {
    return (void *)(phys + hhdm_offset);
}

// Convert virtual address to physical (reverse HHDM)
static inline uint64_t virt_to_phys(void *virt) {
    return (uint64_t)virt - hhdm_offset;
}

// Get page table entry at a specific level
static uint64_t *vmm_get_or_create_table(uint64_t *table, size_t index, bool create) {
    // Check if entry is present
    if (!(table[index] & VMM_PRESENT)) {
        if (!create) {
            return NULL;
        }
        
        // Allocate a new page table
        uint64_t new_table_phys = pmm_alloc();
        if (new_table_phys == 0) {
            serial_write("[VMM] ERROR: Failed to allocate page table\n");
            return NULL;
        }
        
        // Clear the new table
        uint64_t *new_table_virt = (uint64_t *)phys_to_virt(new_table_phys);
        memset(new_table_virt, 0, PAGE_SIZE);
        
        // Set the entry with present, writable, and user flags
        table[index] = new_table_phys | VMM_PRESENT | VMM_WRITABLE | VMM_USER;
    }
    
    // Return virtual address of the table
    uint64_t table_phys = PTE_GET_ADDR(table[index]);
    return (uint64_t *)phys_to_virt(table_phys);
}

void vmm_init(void) {
    serial_write("[VMM] Initializing virtual memory manager...\n");
    
    // Get HHDM offset from Limine
    if (!hhdm_request.response) {
        serial_write("[VMM] ERROR: HHDM not available\n");
        return;
    }
    hhdm_offset = hhdm_request.response->offset;
    
    serial_write("[VMM] HHDM offset: ");
    serial_write_hex(hhdm_offset);
    serial_write("\n");
    
    // Allocate PML4 for kernel address space
    uint64_t pml4_phys = pmm_alloc();
    if (pml4_phys == 0) {
        serial_write("[VMM] ERROR: Failed to allocate kernel PML4\n");
        return;
    }
    
    kernel_address_space.pml4_phys = pml4_phys;
    kernel_address_space.pml4_virt = (uint64_t *)phys_to_virt(pml4_phys);
    kernel_address_space.ref_count = 1;
    
    // Clear the PML4
    memset(kernel_address_space.pml4_virt, 0, PAGE_SIZE);
    
    // Set as current address space
    current_address_space = &kernel_address_space;
    
    serial_write("[VMM] Kernel PML4 allocated at physical: ");
    serial_write_hex(pml4_phys);
    serial_write("\n");
    
    // Note: We keep using Limine's page tables for now
    // The kernel will continue to use higher-half mapping provided by Limine
    // We'll switch to our own page tables when we implement user mode
    
    serial_write("[VMM] Virtual memory manager initialized\n");
}

address_space_t *vmm_create_address_space(void) {
    // Allocate address space structure
    address_space_t *as = (address_space_t *)kmalloc(sizeof(address_space_t));
    if (!as) {
        serial_write("[VMM] ERROR: Failed to allocate address space structure\n");
        return NULL;
    }
    
    // Allocate PML4
    uint64_t pml4_phys = pmm_alloc();
    if (pml4_phys == 0) {
        serial_write("[VMM] ERROR: Failed to allocate PML4\n");
        kfree(as);
        return NULL;
    }
    
    as->pml4_phys = pml4_phys;
    as->pml4_virt = (uint64_t *)phys_to_virt(pml4_phys);
    as->ref_count = 1;
    
    // Clear PML4
    memset(as->pml4_virt, 0, PAGE_SIZE);
    
    // Copy kernel mappings (top half) from kernel address space
    // This ensures kernel is always mapped in all address spaces
    for (size_t i = 256; i < 512; i++) {
        as->pml4_virt[i] = kernel_address_space.pml4_virt[i];
    }
    
    return as;
}

void vmm_destroy_address_space(address_space_t *as) {
    if (!as || as == &kernel_address_space) {
        return;  // Don't destroy kernel address space
    }
    
    as->ref_count--;
    if (as->ref_count > 0) {
        return;  // Still referenced
    }
    
    // Free all user-space page tables (bottom half of PML4)
    for (size_t pml4_i = 0; pml4_i < 256; pml4_i++) {
        if (!(as->pml4_virt[pml4_i] & VMM_PRESENT)) continue;
        
        uint64_t *pdpt = (uint64_t *)phys_to_virt(PTE_GET_ADDR(as->pml4_virt[pml4_i]));
        
        for (size_t pdpt_i = 0; pdpt_i < 512; pdpt_i++) {
            if (!(pdpt[pdpt_i] & VMM_PRESENT)) continue;
            
            uint64_t *pd = (uint64_t *)phys_to_virt(PTE_GET_ADDR(pdpt[pdpt_i]));
            
            for (size_t pd_i = 0; pd_i < 512; pd_i++) {
                if (!(pd[pd_i] & VMM_PRESENT)) continue;
                if (pd[pd_i] & VMM_HUGE) continue;  // 2MB page, no PT
                
                uint64_t *pt = (uint64_t *)phys_to_virt(PTE_GET_ADDR(pd[pd_i]));
                pmm_free(virt_to_phys(pt));
            }
            
            pmm_free(virt_to_phys(pd));
        }
        
        pmm_free(virt_to_phys(pdpt));
    }
    
    // Free PML4
    pmm_free(as->pml4_phys);
    
    // Free address space structure
    kfree(as);
}

bool vmm_map_page(address_space_t *as, uint64_t virt, uint64_t phys, uint64_t flags) {
    if (!as) {
        as = current_address_space;
    }
    
    // Ensure addresses are page-aligned
    virt = PAGE_ALIGN_DOWN(virt);
    phys = PAGE_ALIGN_DOWN(phys);
    
    // Get indices
    size_t pml4_i = PML4_INDEX(virt);
    size_t pdpt_i = PDPT_INDEX(virt);
    size_t pd_i = PD_INDEX(virt);
    size_t pt_i = PT_INDEX(virt);
    
    // Walk page tables, creating them as needed
    uint64_t *pdpt = vmm_get_or_create_table(as->pml4_virt, pml4_i, true);
    if (!pdpt) return false;
    
    uint64_t *pd = vmm_get_or_create_table(pdpt, pdpt_i, true);
    if (!pd) return false;
    
    uint64_t *pt = vmm_get_or_create_table(pd, pd_i, true);
    if (!pt) return false;
    
    // Set the page table entry
    pt[pt_i] = phys | flags | VMM_PRESENT;
    
    // Invalidate TLB entry
    vmm_invlpg(virt);
    
    return true;
}

bool vmm_unmap_page(address_space_t *as, uint64_t virt) {
    if (!as) {
        as = current_address_space;
    }
    
    virt = PAGE_ALIGN_DOWN(virt);
    
    // Get indices
    size_t pml4_i = PML4_INDEX(virt);
    size_t pdpt_i = PDPT_INDEX(virt);
    size_t pd_i = PD_INDEX(virt);
    size_t pt_i = PT_INDEX(virt);
    
    // Walk page tables (don't create)
    uint64_t *pdpt = vmm_get_or_create_table(as->pml4_virt, pml4_i, false);
    if (!pdpt) return false;
    
    uint64_t *pd = vmm_get_or_create_table(pdpt, pdpt_i, false);
    if (!pd) return false;
    
    uint64_t *pt = vmm_get_or_create_table(pd, pd_i, false);
    if (!pt) return false;
    
    // Clear the entry
    pt[pt_i] = 0;
    
    // Invalidate TLB
    vmm_invlpg(virt);
    
    return true;
}

uint64_t vmm_get_physical(address_space_t *as, uint64_t virt) {
    if (!as) {
        as = current_address_space;
    }
    
    virt = PAGE_ALIGN_DOWN(virt);
    
    size_t pml4_i = PML4_INDEX(virt);
    size_t pdpt_i = PDPT_INDEX(virt);
    size_t pd_i = PD_INDEX(virt);
    size_t pt_i = PT_INDEX(virt);
    
    uint64_t *pdpt = vmm_get_or_create_table(as->pml4_virt, pml4_i, false);
    if (!pdpt) return 0;
    
    uint64_t *pd = vmm_get_or_create_table(pdpt, pdpt_i, false);
    if (!pd) return 0;
    
    // Check for 1GB huge page
    if (pd[pd_i] & VMM_HUGE) {
        return PTE_GET_ADDR(pd[pd_i]);
    }
    
    uint64_t *pt = vmm_get_or_create_table(pd, pd_i, false);
    if (!pt) return 0;
    
    if (!(pt[pt_i] & VMM_PRESENT)) {
        return 0;
    }
    
    return PTE_GET_ADDR(pt[pt_i]);
}

bool vmm_is_mapped(address_space_t *as, uint64_t virt) {
    return vmm_get_physical(as, virt) != 0;
}

void vmm_switch_address_space(address_space_t *as) {
    if (!as) {
        return;
    }
    
    current_address_space = as;
    
    // Load CR3 with new page table
    __asm__ volatile("mov %0, %%cr3" :: "r"(as->pml4_phys) : "memory");
}

address_space_t *vmm_get_current_space(void) {
    return current_address_space;
}

address_space_t *vmm_get_kernel_space(void) {
    return &kernel_address_space;
}

void vmm_invlpg(uint64_t virt) {
    __asm__ volatile("invlpg (%0)" :: "r"(virt) : "memory");
}

void vmm_flush_tlb(void) {
    uint64_t cr3;
    __asm__ volatile("mov %%cr3, %0" : "=r"(cr3));
    __asm__ volatile("mov %0, %%cr3" :: "r"(cr3) : "memory");
}

address_space_t *vmm_clone_address_space(address_space_t *src) {
    // Create new address space
    address_space_t *dst = vmm_create_address_space();
    if (!dst) {
        return NULL;
    }
    
    // For now, just share the kernel mappings
    // Full CoW implementation would be more complex
    // TODO: Implement copy-on-write for user pages
    
    return dst;
}
